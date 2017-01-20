/*
 * listener.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#include "listener.h"
#include "../fyslib/xsocketutils.hpp"
#include "../fyslib/sysutils.h"
#include "../fyslib/xlog.h"
using namespace fyslib;
#include "consts.hpp"
#include "types.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include "server.h"
#include "global.h"

namespace tcpserver{

void Listener::Run()
{
	SetFreeOnTerminate(true);
	m_socket = CreateTCPSocket();
	if (-1 == m_socket)
	{
		if (m_log)
		{
			LOG_ERR(m_log,FormatString("create listen socket fail.errno %d",errno).c_str());
			m_log->SwitchQueue();
		}
		return;
	}

	if (-1 == SetSocketNonblock(m_socket))
	{
		LOG_ERR(_log,FormatString("set listen socket nonblock failed.errno:",errno).c_str());
		m_log->SwitchQueue();
		return;
	}
	SetSocketReuseAddr(m_socket,1);
//	int port = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LISTEN_PORT,CFGV_DEFAULT_LISTEN_PORT);
//	if (port < 1)
//		port = CFGV_DEFAULT_LISTEN_PORT;
	if(-1 == TcpBind(m_socket,m_cfg->Get(CFG_SERVER_SECTION,CFG_SERVER_BIND_IP,CFGV_DEFAULT_SERVER_BIND_IP).c_str(),m_port))
	{
		LOG_ERR(_log,FormatString("bind listen socket failed.errno:",errno).c_str());
		m_log->SwitchQueue();
		return;
	}
	sockaddr_in addr_tmp;
	unsigned int addr_len_tmp = sizeof(sockaddr_in);
	getsockname(m_socket,(sockaddr*)&addr_tmp,&addr_len_tmp);
	m_port = ntohs(addr_tmp.sin_port);

	if(-1 == listen(m_socket,m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LISTENQUEUE_LENGTH,CFGV_DEFAULT_SERVER_LISTENQUEUE_LENGTH)))
	{
		LOG_ERR(_log,FormatString("listen failed.errno:",errno).c_str());
		m_log->SwitchQueue();
		return;
	}

	LOG_INFO(_log,FormatString("listening port: %d",m_port).c_str());
	while (!GetTerminated())
	{
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int skt = AcceptTimeout(m_socket,&tv,(sockaddr*)&addr,&addr_len);
		if(-1 == skt)
		{
			//LOG_ERR(m_log,FormatString("accept socket fail,errno %d",errno));
			continue;
		}
		int keepAlive = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_HEARTBEAT,CFGV_DEFAULT_SERVER_HEARTBEAT);
		int keepAliveSnd = m_cfg->GetInt(CFG_SERVER_SECTION,CFG_SERVER_HEARTBEAT_INTERVAL_SECOND,CFGV_DEFAULT_SERVER_HEARTBEAT_INTERVAL_SECOND);
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
			SetSocketLinger(skt,1,linger);
		}
		Client *clt = m_svr->m_client_manager->NewClient(skt,m_port);
		struct epoll_event evt;
		evt.data.ptr = (void*)clt;
		evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
		if(-1 == epoll_ctl(m_epollfd,EPOLL_CTL_ADD,skt,&evt))
		{
			LOG_ERR(m_log,FormatString("epOll_clt failed,errno:%d",errno).c_str());
			close(skt);
		}
		if (m_svr->m_on_client_connected)
			m_svr->m_on_client_connected(clt);
	}
}


Listener::Listener(TcpServer *svr, XLog *log, XConfig *cfg, int epoll_fd, ushort port):
		m_svr(svr),m_log(log),m_cfg(cfg),m_epollfd(epoll_fd),m_socket(-1),m_port(port)
{
}



}//end of namespace

