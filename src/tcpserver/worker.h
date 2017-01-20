/*
 * epollworker.h
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <sys/epoll.h>
#include "../fyslib/tthread.h"
#include "../fyslib/xlog.h"
#include "../fyslib/pandc.hpp"
using namespace fyslib;
#include "types.hpp"
#include "client.h"

namespace tcpserver{

class Worker: public TThread
{
protected:
	void Run();
private:
	struct epoll_event m_evlist[24];
	int m_epoll_fd;
	TcpServer *m_svr;
	XLog *m_log;
	Pandc<ClientBuf> *m_recv_queue;
public:
	Worker(TcpServer *svr,int epoll_fd,XLog *log, Pandc<ClientBuf> *recv_queue);
	~Worker();
};


}//end of tcpserver

#endif /* WORKER_H_ */
