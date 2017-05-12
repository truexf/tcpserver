/*
 * server.h
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#include <vector>
using std::string;
using std::vector;
#include "../fyslib/xconfig.h"
#include "../fyslib/xlog.h"
#include "../fyslib/tthread.h"
#include "../fyslib/pandc.hpp"
using namespace fyslib;
#include "types.hpp"
#include "global.h"
#include "client.h"

namespace tcpserver{

class TcpServer
{
private:
    Listener *m_listener;
    XConfig *m_config;
    vector<Worker*> m_workers;
    vector<Listener*> m_listeners; //listners started by startlistener()
    string m_cfg_file;
    string m_cfg;
    bool m_selflog;
private:
    TcpServer& operator = (const TcpServer&);
    TcpServer(const TcpServer&);
public:
    OnClientConnected m_on_client_connected;
    OnClientDisconnected m_on_client_disconnected;
    OnStartup m_on_starup;
    OnShutdown m_on_shutdown;
    OnError m_on_error;

    ClientManager *m_client_manager;
    XLog *m_log;
    int m_epoll_fd;

    bool Startup();
    Listener* StartListener(ushort port);
    bool Shutdown(bool force);
    bool ReloadConfig();

    string GetStatus();
    ushort GetListenPort(size_t lsnIndex);
    ushort GetListenPort();
public:
    TcpServer(const char* cfg_file, XLog *log);
    TcpServer(const string &cfg, XLog *log);
    virtual ~TcpServer();
};

}//end of namespace

#endif /* SERVER_H_ */
