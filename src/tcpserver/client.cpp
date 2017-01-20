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

namespace tcpserver{

Client::Client(int socket_fd,TcpServer *svr,ushort listen_port): m_socket(socket_fd),m_on_recved(NULL),m_on_sent(NULL),m_close_mark(false),m_svr(svr),m_listen_port(listen_port)
{
	m_uuid = CreateGUID();
	m_data = NULL;
	m_send_queue_lock = CreateMutex(true);
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
	if (m_send_queue_lock && m_on_sent)
	{
		AutoMutex auto1(m_send_queue_lock);
		while (!m_send_queue.empty())
		{
			SendBuf buf(m_send_queue.front());
			m_on_sent(this,buf.m_buf,buf.m_len,false);
		}
		m_send_queue.clear();
	}
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

string Client::GetRemoteIP()
{
	sockaddr_in addr;
	uint len = sizeof(sockaddr_in);
	getpeername(m_socket,(sockaddr*)&addr,&len);
	return string(inet_ntoa(addr.sin_addr));
}

bool Client::Send(void *buf,size_t len) //return value is used to determ if buf is push into the queue, if true,caller should free buf in onsent event.
{
	if (m_send_queue_lock)
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
				int sz = send(m_socket,buf_tmp,remain,0);
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
					wouldblock = true;
					break;
				}
				else
				{
					remain -= sz;
					buf.m_pos += sz;
					IncPtr(&buf_tmp,sz);
				}
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
			{
				m_close_mark = true;
				m_svr->m_client_manager->CloseClient(this);
			}
			if (wouldblock)
				break;
		}
		return true;
	}
	else
		return false;
}

ClientManager::ClientManager(TcpServer *svr):m_svr(svr)
{
	m_client_pool = new ObjectPool<Client>;
	m_lock = CreateMutex(false);
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
			AutoMutex auto1(m_lock);
			vector<string> ccs;
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
			for (vector<string>::iterator it = ccs.begin(); it != ccs.end(); it++)
			{
				m_recycling_pool.erase(*it);
			}
		}
	}
}

ClientManager::~ClientManager()
{

}

void ClientManager::CloseClient(Client *c)
{
	if (!c)
		return;
	AutoMutex au1(m_lock);
	map<string, ClosedClient>::iterator it = m_recycling_pool.find(c->m_uuid);
	if (it == m_recycling_pool.end())
	{
		struct epoll_event evt;
		evt.data.ptr = NULL;
		evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
		epoll_ctl(m_svr->m_epoll_fd,EPOLL_CTL_DEL,c->m_socket,&evt);
		close(c->m_socket);
		c->ClearSendQueue();
		ClosedClient cc(c);
		m_recycling_pool[c->m_uuid] = ClosedClient(c);
		if (m_svr->m_on_client_disconnected)
			m_svr->m_on_client_disconnected(c);
	}
}

Client* ClientManager::NewClient(int socket_fd, ushort listen_port)
{
	AutoMutex au1(m_lock);
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
