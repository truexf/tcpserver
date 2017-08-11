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
#include "../fyslib/tthread.h"
#include "../fyslib/xtimer.h"
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
	m_client_manager = NULL;
	m_epoll_fd = -1;
	m_selflog = false;
}
TcpServer::TcpServer(const string &cfg, XLog *log)
:m_on_client_connected(NULL),m_on_client_disconnected(NULL),m_on_error(NULL),m_on_shutdown(NULL),m_on_starup(NULL),m_listener(NULL),m_config(NULL),m_log(log)
{
	m_cfg.assign(cfg);
	m_client_manager = NULL;
	m_epoll_fd = -1;
	m_selflog = false;
}

TcpServer::~TcpServer()
{

}
bool TcpServer::Shutdown(bool force)
{
    timespec ts;
    ts.tv_nsec = 0;
    ts.tv_sec = 2;

    m_listener->Stop();
    if (!m_listeners.empty()) {
        for(size_t i = 0; i < m_listeners.size(); i++) {
            m_listeners[i]->Stop();
        }
    }
    SleepExactly(&ts);
    for (int i = 0; i < m_workers.size(); i++) {
        m_workers[i]->Stop();
    }
    SleepExactly(&ts);
    close(m_epoll_fd);
    SleepExactly(&ts);
    m_client_manager->Stop();
    ts.tv_sec = 3;
    SleepExactly(&ts);
    if (m_selflog) {
        m_log->SwitchQueue();
        m_log->Stop();
        SleepExactly(&ts);
    }
    return true;
}
bool TcpServer::Startup(int lsnFd)
{
	m_config = new XConfig();
	if (m_cfg.empty())
		m_config->LoadFromFile(m_cfg_file);
	else
		m_config->LoadFromString(m_cfg);
	if (!m_log)
	{
		string log_path = m_config->Get(CFG_SERVER_SECTION,CFG_SERVER_LOG_PATH,ExtractFilePath(ParamStr(0),true));
		int log_queue_len = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LOG_QUEUE_LENGTH,-1);
		m_log = new XLog(log_path.c_str(),log_queue_len);
		m_log->SetFreeOnTerminate(true);
		m_log->Start();
		m_selflog = true;
	}
	m_epoll_fd = epoll_create(1000);
	if (-1 == m_epoll_fd)
	{
		LOG_ERR(_log,FormatString("create epoll fail.errno %d",errno).c_str());
		return false;
	}
	LOG_INFO(_log,"create epoll success.");
	m_client_manager = new ClientManager(this);
	m_client_manager->SetFreeOnTerminate(true);
	m_client_manager->Start();
	int worker_count = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_WORKER_COUNT,CFGV_DEFAULT_SERVER_WORKER_COUNT);
	for (int i = 0; i < worker_count; i++)
	{
		Worker *w = new Worker(this,m_epoll_fd,m_log,m_config);
		w->SetFreeOnTerminate(true);
		m_workers.push_back(w);
		w->Start();
	}
	int port = m_config->GetInt(CFG_SERVER_SECTION,CFG_SERVER_LISTEN_PORT,CFGV_DEFAULT_LISTEN_PORT);

	if (-1 == lsnFd) {
	    m_listener = new Listener(this,_log,m_config,m_epoll_fd,port);
	} else {
	    //(int listenFd, TcpServer *svr, XLog *log, XConfig *cfg, int epoll_fd)
	    m_listener = new Listener(lsnFd, this, _log, m_config, m_epoll_fd);
	    LOG_INFO(_log, FormatString("reuse listnfd %d", lsnFd).c_str());
	}
	m_listener->SetFreeOnTerminate(true);
	m_listener->Start();
	timespec ts;
	ts.tv_nsec = 0;
	ts.tv_sec = 1;
	SleepExactly(&ts);
	if (!m_listener->IsListening()) {
	    Shutdown(true);
	    return false;
	}
	LOG_INFO(_log,"tcp server started.");
	return true;
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

bool TcpServer::ReloadConfig()
{
	return true;
}
ushort TcpServer::GetListenPort() {
    return m_listener->GetListenPort();
}
int TcpServer::GetListenFd() {
    return m_listener->GetListenFd();
}
ushort TcpServer::GetListenPort(size_t lsnIndex)
{
	if (m_listeners.size() <= lsnIndex) {
		return 0;
	}
	return m_listeners[lsnIndex]->GetListenPort();
}

string TcpServer::GetStatus()
{
	return FormatString("pool/recycling: %d/%d\r\n",m_client_manager->PoolSize(),m_client_manager->RecyclingSize());
}

}//end of namespace
