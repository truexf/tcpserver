/*
 * tthread.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: root
 */

#include "tthread.h"
#include <pthread.h>

namespace fyslib{
pthread_mutex_t *CreateMutex(bool recursive)
{
	int iret;
	pthread_mutex_t *m_mutex = new pthread_mutex_t;
	pthread_mutexattr_t *m_mutex_attr = new pthread_mutexattr_t;
	iret = pthread_mutexattr_init(m_mutex_attr);
	if(iret != 0)
	{
		return NULL;
	}
	if (recursive)
		iret = pthread_mutexattr_settype(m_mutex_attr,PTHREAD_MUTEX_RECURSIVE);
	else
		iret = pthread_mutexattr_settype(m_mutex_attr,PTHREAD_MUTEX_NORMAL);
	if(iret != 0)
	{
		pthread_mutexattr_destroy(m_mutex_attr);
		return NULL;
	}
	iret = pthread_mutex_init(m_mutex,m_mutex_attr);
	if(iret != 0)
	{
		pthread_mutexattr_destroy(m_mutex_attr);
		return NULL;
	}
	pthread_mutexattr_destroy(m_mutex_attr);
	delete m_mutex_attr;
	return m_mutex;
}

int SemWait(sem_t *sem, const timespec *timespan) {
    if (!sem)
        return -1;
    if (NULL == timespan || timespan->tv_nsec < 0 || timespan->tv_sec < 0 || timespan->tv_nsec > 1000000000) {
        return sem_wait(sem);
    }

    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return -1;
    }
    ts.tv_nsec += timespan->tv_nsec;
    if (ts.tv_nsec > 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
    }
    ts.tv_sec += timespan->tv_sec;
    for(;;) {
        int ret = sem_timedwait(sem, &ts);
        if (0==ret || ETIMEDOUT == errno) {
            return ret;
        }
    }
    return -1;
}

TThread::TThread()
{
	f_terminated = false;
	f_FreeOnTerminate = false;
	f_started = false;
	f_ThreadID = 0;

	f_init_ok = false;
	int iret;
    iret = pthread_mutexattr_init(&f_mutex_attr);
    if(iret != 0)
    {
        return;
    }
    iret = pthread_mutexattr_settype(&f_mutex_attr,PTHREAD_MUTEX_NORMAL);
    if(iret != 0)
    {
        pthread_mutexattr_destroy(&f_mutex_attr);
        return ;
    }
    iret = pthread_mutex_init(&f_mutex,&f_mutex_attr);
    if(iret != 0)
    {
        pthread_mutexattr_destroy(&f_mutex_attr);
        return ;
    }
    iret = pthread_attr_init(&f_thread_attr);
    if(iret != 0)
    {
        pthread_mutexattr_destroy(&f_mutex_attr);
        pthread_mutex_destroy(&f_mutex);
        return ;
    }
    iret = pthread_attr_setdetachstate(&f_thread_attr,PTHREAD_CREATE_DETACHED);
    if(iret != 0)
    {
        pthread_attr_destroy(&f_thread_attr);
        pthread_mutexattr_destroy(&f_mutex_attr);
        pthread_mutex_destroy(&f_mutex);
        return ;
    }

    f_init_ok = true;


}

TThread::~TThread()
{
	if(f_started)
	{
		pthread_attr_destroy(&f_thread_attr);
		pthread_mutexattr_destroy(&f_mutex_attr);
		pthread_mutex_destroy(&f_mutex);
	}
}

void * TThread::Wrapper(void * Param)
{
	TThread *thrd = (TThread*)Param;
	thrd->f_started = true;
    thrd->Run();
    try{
        if (thrd->GetFreeOnTerminate())
            delete thrd;
    }
    catch(...){}
    return 0;
}

bool TThread::Start()
{
	return f_init_ok && (0 == pthread_create(&f_ThreadID,&f_thread_attr,Wrapper,(void*)this));
}

void TThread::Stop()
{
	if(pthread_mutex_lock(&f_mutex) != 0)
		return;
	f_terminated = true;
	pthread_mutex_unlock(&f_mutex);
}

bool TThread::GetTerminated()
{
	if(pthread_mutex_lock(&f_mutex) != 0)
			return false;
	AutoFuncClass auto1(AutoFunc(pthread_mutex_unlock,&f_mutex));
	return f_terminated;
}

bool TThread::GetFreeOnTerminate()
{
	if(pthread_mutex_lock(&f_mutex) != 0)
			return false;
	AutoFuncClass auto1(AutoFunc(pthread_mutex_unlock,&f_mutex));
	return f_FreeOnTerminate;
}

void TThread::SetFreeOnTerminate(bool v)
{
	if(pthread_mutex_lock(&f_mutex) != 0)
			return;
	AutoFuncClass auto1(AutoFunc(pthread_mutex_unlock,&f_mutex));
	f_FreeOnTerminate = v;
}


}
