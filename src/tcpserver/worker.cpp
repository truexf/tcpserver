
/*
 * worker.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */
#include "worker.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "client.h"
#include <stdlib.h>
#include <vector>
using std::vector;
#include "global.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/xsocketutils.hpp"
using namespace fyslib;
#include "consts.hpp"
#include "server.h"

namespace tcpserver{

struct epollInfo {
    map<Client*, bool> in;
    map<Client*, bool> out;
    void Clear() {
        in.clear();
        out.clear();
    }
};

void Worker::Run()
{
	int ready;
	Client *clt = NULL;
	epollInfo info;
	while (!GetTerminated())
	{
		ready = epoll_wait(m_epoll_fd,m_evlist,sizeof(m_evlist)/sizeof(m_evlist[0]),1000);
		if(-1 == ready)
		{
			if(EINTR == errno)
				continue;
			else
			{
				LOG_ERR(m_log,FormatString("epoll_wait fail,errno: %d",errno).c_str());
				break;
			}
		}
		if(0 == ready)
			continue;

		info.Clear();
		for(int i=0;i<ready;++i)
		{
			clt = (Client*)(m_evlist[i].data.ptr);
			if (clt->m_is_listener) {
                if(m_evlist[i].events & (EPOLLHUP | EPOLLERR))
                {
                    // close socket epoll_ctl_del
                    LOG_ERR(m_log,"listen socket closed.");
                    //m_svr->m_client_manager->CloseClient(clt);
                    continue;
                }
                for(;;) {
                    sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    int skt = accept(clt->m_socket,(sockaddr*)&addr,&addr_len);
                    if(-1 == skt)
                    {
                        //LOG_ERR(m_log,FormatString("accept socket fail,errno %d",errno).c_str());
                        break;
                    } else if (0 == skt) {
                        LOG_INFO(m_log,"select timeout.");
                        break; //timeout
                    }
                    //SetSocketReuseAddr(skt,0);
                    int keepAlive = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_HEARTBEAT,CFGV_DEFAULT_SERVER_HEARTBEAT);
                    int keepAliveSnd = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_HEARTBEAT_INTERVAL_SECOND,CFGV_DEFAULT_SERVER_HEARTBEAT_INTERVAL_SECOND);
                    LOG_INFO(m_log,Fmt("keepalive %d, keepalivesnd %d",keepAlive,keepAliveSnd).c_str());
                    SetSocketKeepalive(skt,keepAlive,keepAliveSnd);
                    SetSocketNonblock(skt);
                    int recv_buf_size = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_RECVBUF_SIZE,-1);
                    int send_buf_size = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_SENDBUF_SIZE,-1);
                    if (send_buf_size > 0)
                        SetSocketSendbufSize(skt,send_buf_size);
                    if (recv_buf_size > 0)
                        SetSocketSendbufSize(skt,recv_buf_size);
                    int linger = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LINGER,-1);
                    if (linger >= 0)
                    {
                        LOG_INFO(m_log, Fmt("SetLinger %d", linger).c_str());
                        SetSocketLinger(skt,1,linger);
                    }
                    Client *c = m_svr->m_client_manager->NewClient(skt,clt->m_listen_port);
                    if (m_svr->m_on_client_connected)
                        m_svr->m_on_client_connected(c);
                    struct epoll_event evt;
                    evt.data.ptr = (void*)c;
                    evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
                    if(-1 == epoll_ctl(m_epoll_fd,EPOLL_CTL_ADD,skt,&evt))
                    {
                        LOG_ERR(m_log,FormatString("epOll_clt failed,errno:%d",errno).c_str());
                        close(skt);
                    }
                }
			} else {
                if(m_evlist[i].events & EPOLLRDHUP)
                {
                    // peer shutdown close socket epoll_ctl_del
                    LOG_INFO(m_log,"client closed.");
                    m_svr->m_client_manager->CloseClient(clt);

                    continue;
                }
                if(m_evlist[i].events & (EPOLLHUP | EPOLLERR))
                {
                    // close socket epoll_ctl_del
                    LOG_INFO(m_log,"client closed.");
                    m_svr->m_client_manager->CloseClient(clt);
                    continue;
                }
                if(m_evlist[i].events & EPOLLIN)
                {
                    info.in[clt] = true;
                    //clt->Recv();
                }
                if(m_evlist[i].events & EPOLLOUT)
                {
                    info.out[clt] = true;
                    //clt->Send(NULL,0);
                }
			}
		}

		//执行轮转io，避免部分socket io忙时导致其他socket io饥饿
		bool in = true;
		bool out = true;
		while (!info.in.empty() || !info.out.empty()) {
		    if (in) {
		        in = false;
                for (map<Client*,bool>::iterator it = info.in.begin(); it != info.in.end(); it++) {
                    if (it->second) {
                        bool again = it->first->Recv(30000);
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
                for (map<Client*,bool>::iterator it = info.out.begin(); it != info.out.end(); it++) {
                    if (it->second) {
                        bool again = it->first->Send(NULL,0,30000);
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

Worker::Worker(TcpServer* svr, int epoll_fd, XLog* log, XConfig *cfg):
		m_svr(svr),m_epoll_fd(epoll_fd),m_log(log),m_cfg(cfg)
{

}

Worker::~Worker()
{

}


} //end of namespace

