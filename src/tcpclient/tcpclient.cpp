#include "tcpclient.h"
#include "../fyslib/xlog.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/xsocketutils.hpp"
#include "../fyslib/AutoObject.hpp"
#include "../fyslib/xtimer.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

void TcpClientManager::Init(XLog *log, int worker_count)
{
    m_worker_count = worker_count;
    m_log = log;
}

bool TcpClientManager::Go()
{
    m_epoll_fd = epoll_create(1000);
    if (-1 == m_epoll_fd)
    {
        LOG_ERR(_log,FormatString("create epoll fail.errno %d",errno).c_str());
        return false;
    }
    for (int i = 0; i < m_worker_count; i++)
    {
        TcpClientWorker *w = new TcpClientWorker;
        w->SetMgr(this);
        w->Start();
        m_workers.push_back(w);
    }
    m_clients_lock = CreateMutex(true);
    m_workers_lock = CreateMutex(true);
    m_closed_clients_lock = CreateMutex(true);
    Start();
    return true;
}

TcpClient * TcpClientManager::ConnectRemote(timeval *tv, string ip, unsigned short port, OnTcpClientDataRecved onRecved, OnTcpClientDataSent onSent, OnTcpClientError onError, void *setData)
{
    int skt = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (-1 == skt)
    {
        LOG_ERR(m_log,FormatString("create socket fail,%d",errno).c_str());
        return NULL;
    }
    sockaddr_in addr;
    memset(&addr,0,sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    if (0 != TcpConnectTimeout(skt,tv,(sockaddr*)&addr,sizeof(sockaddr_in))) {
        close(skt);
        LOG_WARN(m_log,FormatString("connect to %s:%d fail,%d",ip.c_str(),port,errno).c_str());
        return NULL;
    }

    TcpClient *c = new TcpClient();
    c->m_uuid = CreateGUID();
    c->SetMgr(this);
    c->SetSocket(skt);
    c->m_remote_ip = ip;
    c->m_remote_port = port;
    c->SetData(setData);
    {
        AutoMutex auto1(m_clients_lock);
        m_clients[c->m_uuid] = c;
    }
    c->m_on_data_recved = onRecved;
    c->m_on_data_sent = onSent;
    c->m_on_error = onError;
    struct epoll_event evt;
    evt.data.ptr = (void*)c;
    evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    if(-1 == epoll_ctl(m_epoll_fd,EPOLL_CTL_ADD,skt,&evt))
    {
        LOG_ERR(m_log,FormatString("epOll_clt failed,errno:%d",errno).c_str());
        close(skt);
        AutoMutex auto1(m_clients_lock);
        m_clients.erase(c->m_uuid);
        delete c;
        return NULL;
    }
    //c->Recv();
    return c;
}

void TcpClientManager::RemoveClient(TcpClient *c)
{
    if (!c)
        return;
    AutoMutex auto1(m_closed_clients_lock);
    AutoMutex auto2(m_clients_lock);
    map<string, ClosedTcpClient>::iterator it = m_closed_clients.find(c->m_uuid);
    if (it == m_closed_clients.end())
        return;
    map<string, TcpClient*>::iterator it2 = m_clients.find(c->m_uuid);
    if (it2 != m_clients.end())
    {
        m_clients.erase(it2);
    }
    //int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    epoll_ctl(m_epoll_fd,EPOLL_CTL_DEL,c->m_socket,NULL);
    close(c->m_socket);
    ClosedTcpClient cc;
    GetLocalTime(&cc.m_close_time);
    cc.m_client = c;
    m_closed_clients[c->m_uuid] = cc;
    return;
}

void TcpClientManager::Run()
{
    while (!GetTerminated())
    {
        timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        SleepExactly(&ts);
        if (GetTerminated())
            break;
        {
            vector<string> cc_tmp;
            AutoMutex auto1(m_closed_clients_lock);
            for (map<string, ClosedTcpClient>::iterator it = m_closed_clients.begin(); it != m_closed_clients.end(); it++)
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                if (it->second.m_client && SecondsBetween(it->second.m_close_time,st) > 10)
                {
                    delete it->second.m_client;
                    cc_tmp.push_back(it->first);
                }
            }
            for (size_t i = 0; i < cc_tmp.size(); i++)
            {
                m_closed_clients.erase(cc_tmp[i]);
            }
        }
    }
}

TcpClient::TcpClient(): m_socket(-1),m_remote_port(0),m_mgr(NULL),m_close_mark(false),m_data(NULL)
{
    m_on_data_recved = NULL;
    m_on_data_sent = NULL;
    m_on_error = NULL;
    m_send_queue_lock = CreateMutex(true);
    m_recv_lock = CreateMutex(true);
    memset(&m_recv_buf,0,sizeof(m_recv_buf));
}

TcpClient::~TcpClient()
{
    if (m_on_data_sent)
    {
        for (deque<ClientSendBuf>::iterator it = m_send_queue.begin(); it != m_send_queue.end(); it++)
        {
            m_on_data_sent(this,it->m_buf,it->m_len);
        }
    }
    DestroyMutex(m_send_queue_lock);
    DestroyMutex(m_recv_lock);
}

bool TcpClient::Send(void *buf,size_t len)
{
    int ret = 0;
    {
        AutoMutex auto1(m_send_queue_lock);
        if (m_close_mark)
            return false;
        if (NULL != buf && len > 0)
            m_send_queue.push_back(ClientSendBuf(buf,len,0));
        while (!m_send_queue.empty())
        {
            ClientSendBuf bufSend(m_send_queue.front());
            m_send_queue.pop_front();
            int remain = bufSend.m_len - bufSend.m_send_pos;
            void *buf_tmp = AddPtr(bufSend.m_buf,bufSend.m_send_pos);
            bool wouldblock = false;
            while (true)
            {
                int sz = send(m_socket,buf_tmp,remain,MSG_NOSIGNAL);
                if (-1 == sz)
                {
                    switch(errno)
                    {
                    case EAGAIN:
                        wouldblock = true;
                        break;
                    default:
                        if (m_on_error) {
                            m_on_error(this, errno);
                        }
                        LOG_WARN(m_mgr->m_log,FormatString("socket send() fail,errno: %d",errno).c_str());
                        wouldblock = true;
                        m_close_mark = true;
                        break;
                    }
                    break;
                }
                else if (0 == sz)
                {
                    LOG_WARN(m_mgr->m_log,"socket send return 0.")
                    wouldblock = true;
                    break;
                }
                else
                {
                    remain -= sz;
                    bufSend.m_send_pos += sz;
                    IncPtr(&buf_tmp,sz);
                    if (remain <= 0) {
                        break;
                    }
                }
                ret += sz;
            }
            if (NULL != m_on_data_sent && remain <= 0)
            {
                m_on_data_sent(this,bufSend.m_buf,bufSend.m_len);
            }
            else
            {
                m_send_queue.push_front(bufSend);
            }
            if (m_close_mark)
                break;
            if (wouldblock)
                break;

            return true;
        }
    }
    if (m_close_mark)
        m_mgr->RemoveClient(this);
    return false;
}

bool TcpClient::Recv()
{
    AutoMutex auto1(m_recv_lock);
    while (true)
    {
        int sz = recv(m_socket,m_recv_buf,sizeof(m_recv_buf),0);
        if (-1 == sz)
        {
            switch(errno)
            {
            case EWOULDBLOCK: //equal to EAGAIN
                break;
            default:
                if (m_on_error) {
                    m_on_error(this, errno);
                }
                LOG_WARN(m_mgr->m_log,FormatString("socket recv() fail,errno: %d",errno).c_str());
                m_mgr->RemoveClient(this);
                break;
            }
            break;
        }
        else if (0==sz)
        {
            if (m_on_error) {
                m_on_error(this, 0);
            }
            //peer shutdown. close socket epoll_ctl_del
            m_mgr->RemoveClient(this);
            break;
        }
        if (m_on_data_recved) {
            m_on_data_recved(this, m_recv_buf, sz);
        }
        if ((size_t)sz < sizeof(m_recv_buf))
            break;
        else
            return true;
    }
    return false;
}

struct epollInfo {
    map<TcpClient*, bool> in;
    map<TcpClient*, bool> out;
    void Clear() {
        in.clear();
        out.clear();
    }
};

void TcpClientWorker::Run()
{
    int ready;
    TcpClient *clt = NULL;
    epollInfo info;
    while (!GetTerminated())
    {
        ready = epoll_wait(m_mgr->m_epoll_fd,m_evlist,sizeof(m_evlist)/sizeof(m_evlist[0]),1000);
        if(-1 == ready)
        {
            if(EINTR == errno)
                continue;
            else
            {
                LOG_ERR(m_mgr->m_log,FormatString("epoll_wait fail,errno: %d",errno).c_str());
                break;
            }
        }
        if(0 == ready)
            continue;

        info.Clear();
        for(int i=0;i<ready;++i)
        {
            clt = (TcpClient*)(m_evlist[i].data.ptr);
            if(m_evlist[i].events & EPOLLRDHUP)
            {
                // peer shutdown close socket epoll_ctl_del
                LOG_INFO(m_mgr->m_log,"client closed.");
                m_mgr->RemoveClient(clt);

                continue;
            }
            if(m_evlist[i].events & (EPOLLHUP | EPOLLERR))
            {
                // close socket epoll_ctl_del
                LOG_INFO(m_mgr->m_log,"client closed.");
                m_mgr->RemoveClient(clt);
                continue;
            }
            if(m_evlist[i].events & EPOLLIN)
            {
                info.in[clt] = true;

            }
            if(m_evlist[i].events & EPOLLOUT)
            {
                info.out[clt] = true;

            }
        }

        //执行轮转io，避免部分socket io忙时导致其他socket io饥饿
        bool in = true;
        bool out = true;
        while (!info.in.empty() || !info.out.empty()) {
            if (in) {
                in = false;
                for (map<TcpClient*,bool>::iterator it = info.in.begin(); it != info.in.end(); it++) {
                    if (it->second) {
                        bool again = it->first->Recv();
                        if (again) {
                            in = true;
                        } else {
                            it->second = false;
                        }
                    }
                }
            }
            if (out) {
                out = false;
                for (map<TcpClient*,bool>::iterator it = info.out.begin(); it != info.out.end(); it++) {
                    if (it->second) {
                        bool again = it->first->Send(NULL,0);
                        if (again) {
                            out = true;
                        } else {
                            it->second = false;
                        }
                    }
                }
            }
            if (!in && !out) {
                break;
            }
        }
    }
}
