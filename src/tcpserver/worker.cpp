
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
#include "global.h"
#include "../fyslib/sysutils.h"
using namespace fyslib;
#include "consts.hpp"
#include "server.h"

namespace tcpserver{

void Worker::Run()
{
	int ready;
	Client *clt = NULL;
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

		for(int i=0;i<ready;++i)
		{
			clt = (Client*)(m_evlist[i].data.ptr);

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
				while (true)
				{
					void *buf = malloc(RECV_BUF_SIZE);
					if (NULL == buf)
					{
						LOG_ERR(m_log,"malloc recv buf fail.");
						break;
					}
					int sz = recv(clt->m_socket,buf,RECV_BUF_SIZE,0);
					if (-1 == sz)
					{
						switch(errno)
						{
						case EWOULDBLOCK: //equal to EAGAIN
							break;
						default:
							LOG_WARN(m_log,FormatString("socket recv() fail,errno: %d",errno).c_str());
							break;
						}
						break;
					}
					else if (0==sz)
					{
						//peer shutdown. close socket epoll_ctl_del
						m_svr->m_client_manager->CloseClient(clt);
						break;
					}
					if (clt->m_on_recved)
					{
						clt->m_on_recved(clt,buf,sz);
						free(buf);
					}
					if (sz < RECV_BUF_SIZE)
						break;
				}
			}
			if(m_evlist[i].events & EPOLLOUT)
			{
				clt->Send(NULL,0);
			}
		}
	}
}

Worker::Worker(TcpServer* svr, int epoll_fd, XLog* log, Pandc<ClientBuf> *recv_queue):
		m_svr(svr),m_epoll_fd(epoll_fd),m_log(log),m_recv_queue(recv_queue)
{

}

Worker::~Worker()
{

}


} //end of namespace

