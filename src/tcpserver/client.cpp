/*
 * clientconnection.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#include "client.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/objectpool.hpp"
#include "../fyslib/xlog.h"
#include "../fyslib/systemtime.h"
#include "../fyslib/xtimer.h"
using namespace fyslib;
#include <unistd.h>
#include "global.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcpserver.h"
#include "worker.h"


namespace tcpserver{

Client::Client(int socket_fd,TcpServer *svr,ushort listen_port): m_socket(socket_fd),m_on_recved(NULL),m_on_sent(NULL),m_close_mark(false),m_svr(svr),m_listen_port(listen_port)
{
    m_is_listener = false;
	m_uuid = CreateGUID();
	m_data = NULL;
	m_send_queue_lock = CreateMutex(true);
	m_recv_lock = CreateMutex(false);
	if (NULL == m_send_queue_lock)
	{
		LOG_ERR(m_svr->m_log,FormatString("create mutex fail,errno %d",errno).c_str());
	}

}

Client::~Client()
{
}

void Client::ClearSendQueue()
{
    AutoMutex auto1(m_send_queue_lock);
    while (!m_send_queue.empty())
    {
        SendBuf buf(m_send_queue.front());
        m_send_queue.pop_front();
        m_on_sent(this,buf.m_buf,buf.m_len,false);
    }
    m_send_queue.clear();

}

void Client::Init(int socket_fd, ushort listen_port)
{
	m_socket = socket_fd;
	m_on_recved = NULL;
	m_on_sent = NULL;
	m_close_mark = false;
	m_uuid = CreateGUID();
	m_data = NULL;
	m_listen_port = listen_port;
}

void Client::Close() {
    m_svr->m_client_manager->CloseClient(this);
}

string Client::GetRemoteIP()
{
	sockaddr_in addr;
	uint len = sizeof(sockaddr_in);
	getpeername(m_socket,(sockaddr*)&addr,&len);
	return string(inet_ntoa(addr.sin_addr));
}

bool Client::Recv(Worker *workerThread,int limit)
{
    int ret = 0;
    AutoMutex auto1(m_recv_lock);
    void *buf = NULL;
    while (true)
    {
        buf = workerThread->PullRecvBuf();//malloc(RECV_BUF_SIZE);
        if (NULL == buf)
        {
            LOG_ERR(m_svr->m_log,"malloc recv buf fail.");
            break;
        }
        int sz = recv(m_socket,buf,RECV_BUF_SIZE,0);
        if (-1 == sz)
        {
            switch(errno)
            {
            case EWOULDBLOCK: //equal to EAGAIN
                break;
            default:
                LOG_WARN(m_svr->m_log,FormatString("socket recv() fail,errno: %d",errno).c_str());
                m_svr->m_client_manager->CloseClient(this);
                break;
            }
            break;
        }
        else if (0==sz)
        {
            //peer shutdown. close socket epoll_ctl_del
            m_svr->m_client_manager->CloseClient(this);
            break;
        }
        if (m_on_recved) {
            m_on_recved(this, buf, sz);
        }
        if (buf) {
            free(buf);
            buf = NULL;
        }
        if (sz < RECV_BUF_SIZE)
            break;
        ret += sz;
        if (limit > 0 && ret >= limit) {
            return true;
        }
    }
    if (buf) {
        workerThread->PushRecvBuf(buf);//free(buf);
        buf = NULL;
    }
    return false;
}

bool Client::Send(void *buf,size_t len, int limit)
{
    int ret = 0;
    {
        AutoMutex auto1(m_send_queue_lock);
        if (m_close_mark)
            return false;
        if (NULL != buf && len > 0)
            m_send_queue.push_back(SendBuf(buf,len,0));
        while (!m_send_queue.empty())
        {
            SendBuf buf(m_send_queue.front());
            m_send_queue.pop_front();
            int remain = buf.m_len - buf.m_pos;
            void *buf_tmp = AddPtr(buf.m_buf,buf.m_pos);
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
                        LOG_WARN(m_svr->m_log,FormatString("socket send() fail,errno: %d",errno).c_str());
                        wouldblock = true;
                        m_close_mark = true;
                        break;
                    }
                    break;
                }
                else if (0 == sz)
                {
                    LOG_WARN(m_svr->m_log,"socket send return 0.")
                    wouldblock = true;
                    break;
                }
                else
                {
                    remain -= sz;
                    buf.m_pos += sz;
                    IncPtr(&buf_tmp,sz);
                    if (remain <= 0) {
                        break;
                    }
                }
                ret += sz;
            }
            if (NULL != m_on_sent && remain <= 0)
            {
                m_on_sent(this,buf.m_buf,buf.m_len,true);
            }
            else
            {
                m_send_queue.push_front(buf);
            }
            if (m_close_mark)
                break;
            if (wouldblock)
                break;
            if (limit >0 && ret >= limit) {
                return true;
            }
        }
    }
    if (m_close_mark)
        m_svr->m_client_manager->CloseClient(this);
    return false;
}

ClientManager::ClientManager(TcpServer *svr):m_svr(svr)
{
	m_client_pool = new ObjectPool<Client>;
	m_recycling_pool_lock = CreateMutex(true);
	m_new_client_lock = CreateMutex(true);
}

void ClientManager::Run()
{
	timespec ts;
	ts.tv_nsec = 0;
	ts.tv_sec = 1;
	while (!GetTerminated())
	{
		SleepExactly(&ts);
		{
		    vector<string> ccs;
		    {
                AutoMutex auto1(m_recycling_pool_lock);
                for (map<string, ClosedClient>::iterator it = m_recycling_pool.begin(); it != m_recycling_pool.end(); it++)
                {
                    ClosedClient cc = it->second;
                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    if (SecondsBetween(cc.m_close_time,st) > 10)
                    {
                        m_client_pool->PushObject(cc.m_client);
                        ccs.push_back(it->first);
                    }
                }
		    }
			for (vector<string>::iterator it = ccs.begin(); it != ccs.end(); it++)
			{
				m_recycling_pool.erase(*it);
			}
		}
	}
}

ClientManager::~ClientManager()
{
    DestroyMutex(m_new_client_lock);
    DestroyMutex(m_recycling_pool_lock);
}

void ClientManager::CloseClient(Client *c)
{
	if (!c)
		return;
	bool bFound = false;
	{
        AutoMutex au1(m_recycling_pool_lock);
        map<string, ClosedClient>::iterator it = m_recycling_pool.find(c->m_uuid);
        if (it == m_recycling_pool.end())
        {
            bFound = true;
            struct epoll_event evt;
            evt.data.ptr = NULL;
            evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
            epoll_ctl(m_svr->m_epoll_fd,EPOLL_CTL_DEL,c->m_socket,&evt);
            close(c->m_socket);
            c->m_socket = -1;
            c->ClearSendQueue();
            m_recycling_pool[c->m_uuid] = ClosedClient(c);
        }
	}
    if (bFound && m_svr->m_on_client_disconnected)
        m_svr->m_on_client_disconnected(c);
    c->SetData(NULL);
}

Client* ClientManager::NewClient(int socket_fd, ushort listen_port)
{
	AutoMutex au1(m_new_client_lock);
	Client *ret = NULL;
	ret = m_client_pool->PullObject();
	if (ret)
		ret->Init(socket_fd,listen_port);
	else
		ret = new Client(socket_fd,m_svr,listen_port);
	return ret;
}

int ClientManager::PoolSize()
{
	return m_client_pool->Size();
}

int ClientManager::RecyclingSize()
{
	return m_recycling_pool.size();
}

} //end of tcpserver
