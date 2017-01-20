/*
 * listener.h
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef LISTENER_H_
#define LISTENER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <string>
using std::string;
#include "../fyslib/tthread.h"
#include "../fyslib/xlog.h"
#include "../fyslib/xconfig.h"
using namespace fyslib;
#include "types.hpp"

namespace tcpserver{

class Listener: public TThread
{
private:
	Listener(const Listener&);
	Listener& operator = (const Listener&);

	int m_socket;
	OnError m_on_error;
	XLog *m_log;
	XConfig *m_cfg;
	int m_epollfd;
	TcpServer *m_svr;
	u_short m_port;
protected:
	void Run(); //override;
public:
	Listener(TcpServer *svr, XLog *log, XConfig *cfg, int epoll_fd,ushort port);
	ushort GetListenPort(){return m_port;}
};

}//end of namespace

#endif /* LISTENER_H_ */
