/*
 * tthread.h
 *
 *  Created on: Jan 26, 2015
 *      Author: root
 */

#ifndef FYSLIB_TTHREAD_H_
#define FYSLIB_TTHREAD_H_

#include <pthread.h>
#include "AutoObject.hpp"
#include <time.h>
#include <errno.h>

namespace fyslib{


pthread_mutex_t *CreateMutex(bool recursive);
inline int LockMutex(pthread_mutex_t *mtx)
{
	return pthread_mutex_lock(mtx);
}

inline int UnlockMutex(pthread_mutex_t *mtx)
{
	return pthread_mutex_unlock(mtx);
}

inline int DestroyMutex(pthread_mutex_t *mtx)
{
	if (NULL == mtx)
		return EINVAL;
	int ret = pthread_mutex_destroy(mtx);
	delete mtx;
	return ret;
}

class TCondition
{
public:
	TCondition(){
		pthread_cond_init(&m_cond,NULL);
		pthread_mutex_init(&m_mtx,NULL);
	}
	~TCondition(){
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mtx);
	}
	int Wait(const struct timespec *t){
		if (NULL == t)
		{
			AutoMutex auto1(&m_mtx);
			return pthread_cond_wait(&m_cond,&m_mtx);
		}
		else
		{
			AutoMutex auto1(&m_mtx);
			struct timespec st;
			clock_gettime(CLOCK_REALTIME,&st);
			st.tv_nsec += t->tv_nsec;
			st.tv_sec += t->tv_sec;
			return pthread_cond_timedwait(&m_cond,&m_mtx,&st);
		}
	}
	int Signal(){
		AutoMutex auto1(&m_mtx);
		return pthread_cond_signal(&m_cond);
	}
	int Broadcast(){
		AutoMutex auto1(&m_mtx);
		return pthread_cond_broadcast(&m_cond);
	}
private:
	pthread_mutex_t m_mtx;
	pthread_cond_t m_cond;
};


/*
封装的线程类
使用方法，从TThread派生一个具体的线程类，然后重写Run()成员函数，在里面写线程要执行的代码，例如：
class TMyThread: public TThread
{
protected:
void Run(){//在这里写线程要执行的代码}
}
...
TMyThread my_thrd = new TMyThread();
my_thrd.Start();

成员说明：
Start()  开始线程执行
SetTerminated()  标记结束线程
GetTerminated()  线程是否标记为结束
SetFreeOnTerminate() 设置线程执行完以后自动是否当前线程对象
GetFreeOnTerminate() 是否自动是否线程对象
GetThreadHandle() 线程句柄
GetThreadID() 线程ID
*/
class TThread
{
private:
    pthread_t f_ThreadID; //线程ID
    pthread_attr_t f_thread_attr;
    pthread_mutex_t f_mutex;
    pthread_mutexattr_t f_mutex_attr;
    bool f_FreeOnTerminate; //线程结束时自动删除线程对象
    bool f_terminated; //mark if terminated
    static void * Wrapper(void *Param);
    TThread(const TThread&);
    TThread& operator=(const TThread&);
public:
    TThread();
    virtual ~TThread();
    bool Start();
    void Stop();
    bool GetTerminated();
    void SetFreeOnTerminate(bool v);
    bool GetFreeOnTerminate();
    pthread_t GetThreadID(){return f_ThreadID;}
    bool f_started; //do not use this member
protected:
    virtual void Run()=0;
};

}

#endif /* FYSLIB_TTHREAD_H_ */
