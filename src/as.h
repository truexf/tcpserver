/*
 * as.h
 *
 *  Created on: Jan 6, 2017
 *      Author: root
 */

#ifndef AS_H_
#define AS_H_

#include "fyslib/sysutils.h"
#include "fyslib/tthread.h"
#include "tcpserver/tcpserver.h"
#include "fyslib/xlog.h"
#include <map>
using namespace fyslib;
using namespace std;
using namespace tcpserver;


bool StartAS(XLog *log);

class AServer
{
private:
	map<ushort, string> m_port_macs;
	pthread_mutex_t *m_port_macs_lock;
	TcpServer *m_svr;
public:
	XLog *m_log;

public:
	AServer();
	~AServer();
	string GetPortMacList();//port=mac,split&port=mac&...
	bool Start(XLog *log);
	void BindPortMac(ushort port,string mac);
	ushort GetPortByMac(string mac);
	string GetMacByPort(ushort port);

};

extern AServer *_aserver;

#endif /* AS_H_ */
