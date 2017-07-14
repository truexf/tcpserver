/*
 * xtimer.cpp
 *
 *  Created on: Mar 11, 2015
 *      Author: root
 */

#include "xtimer.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

namespace fyslib{

int SleepExactly(const struct timespec *t)
{
	struct timespec request;
	if(-1 == clock_gettime(CLOCK_REALTIME,&request))
		return -1;
	request.tv_sec += t->tv_sec;
	request.tv_nsec += t->tv_nsec;
	clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&request,NULL);
	return 0;
}

class _sleep_thread: public TThread
{
public:
	void Set(BaseFuncClass *p,bool runfirst,const struct timespec *t)
	{
		m_proc = p;
		m_runfirst = runfirst;
		memcpy(&m_t,t,sizeof(timespec));
	}
protected:
	BaseFuncClass *m_proc;
	bool m_runfirst;
	timespec m_t;
	void Run()
	{
		if (m_runfirst)
			m_proc->ExecFunc();
		while(!GetTerminated())
		{
			SleepExactly(&m_t);
			m_proc->ExecFunc();
		}
		delete m_proc;
	}
};

class _async_executer: public TThread
{
public:
    void Set(BaseFuncClass *p) {
            m_proc = p;
    }
protected:
    BaseFuncClass *m_proc;
    void Run() {
        m_proc->ExecFunc();
        delete m_proc;
    }
};

TThread *StartAsyncTimer(BaseFuncClass *proc,bool runfirst,const struct timespec *t)
{
	_sleep_thread *thrd = new _sleep_thread;
	thrd->Set(proc,runfirst,t);
	thrd->SetFreeOnTerminate(true);
	thrd->Start();
	return thrd;
}

void AsyncExecute(BaseFuncClass *proc)
{
    _async_executer *thrd = new _async_executer;
    thrd->Set(proc);
    thrd->SetFreeOnTerminate(true);
    thrd->Start();
}



}
