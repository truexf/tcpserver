/*
 * server.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#include "server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include "../fyslib/sysutils.h"
#include "client.h"
#include "consts.hpp"
#include "global.h"
#include "worker.h"
#include "listener.h"

namespace tcpserver{


TcpServer::TcpServer(const char* cfg_file, XLog *log)
:m_on_client_connected(NULL),m_on_client_disconnected(NULL),m_on_error(NULL),m_on_shutdown(NULL),m_on_starup(NULL),m_listener(NULL),m_config(NULL),m_log(log)
{
	m_cfg_file.assign(cfg_file);
}

TcpServer::~TcpServer()
{

}

bool TcpServer::Startup()
{
	m_config = new XConfig();
	m_config->LoadFromFile(m_cfg_file);
	if (!m_log)
	{
		string log_path = m_config->Get(CFG_SERVER_SECTION,CFG_SERVER_LOG_PATH,ExtractFilePath(ParamStr(0),true));
		int log_queue_len = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LOG_QUEUE_LENGTH,-1);
		m_log = new XLog(log_path.c_str(),log_queue_len);
		m_log->Start();
	}
	m_epoll_fd = epoll_create(1000);
	if (-1 == m_epoll_fd)
	{
		LOG_ERR(_log,FormatString("create epoll fail.errno %d",errno).c_str());
		return false;
	}
	m_client_manager = new ClientManager(this);
	m_client_manager->Start();
	m_recv_queue = new Pandc<ClientBuf>;

	int worker_count = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_WORKER_COUNT,CFGV_DEFAULT_SERVER_WORKER_COUNT);
	for (int i = 0; i < worker_count; i++)
	{
		Worker *w = new Worker(this,m_epoll_fd,m_log,m_recv_queue);
		m_workers.push_back(w);
		w->Start();
	}

	int port = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LISTEN_PORT,CFGV_DEFAULT_LISTEN_PORT);
	if (port < 1)
		port = CFGV_DEFAULT_LISTEN_PORT;
	m_listener = new Listener(this,_log,m_config,m_epoll_fd,port);
	m_listener->Start();

	LOG_INFO(_log,"tcp server started.");
}

Listener* TcpServer::StartListener(ushort port)
{
	Listener *lsn = new Listener(this,_log,m_config,m_epoll_fd,port);
	lsn->SetFreeOnTerminate(true);
	bool b = lsn->Start();
	if (b)
	{
		m_listeners.push_back(lsn);
		return lsn;
	}

	return NULL;
}

bool TcpServer::Shutdown(bool force)
{
	return true;
}

bool TcpServer::ReloadConfig()
{
	return true;
}

string TcpServer::GetStatus()
{
	return FormatString("pool/recycling: %d/%d\r\n",m_client_manager->PoolSize(),m_client_manager->RecyclingSize());
}

}//end of namespace
