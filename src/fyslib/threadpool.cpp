/*
 * ThreadPool.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: root
 */

#include "threadpool.h"
#include "xtimer.h"
#include "tthread.h"

namespace fyslib{
class _PoolThread: public TThread
{
public:
	void SetPool(ThreadPool *pool)
	{
		m_pool = pool;
	}
protected:
	void Run() //override;
	{
		m_pool->IncRunningThreadCount();
		SetFreeOnTerminate(true);
		while(!GetTerminated())
		{
			BaseFuncClass* task = m_pool->PullTask();
			task->ExecFunc();
			delete task;
		}
		m_pool->DecRunningThreadCount();
	}
private:
	ThreadPool *m_pool;
};


static int NullProc()
{
	return 1;
}
ThreadPool::ThreadPool(int thread_count)
{
	m_task_queue = new Pandc<BaseFuncClass*>;
	m_thread_count = thread_count;
	m_running_thread_count = 0;
	for(size_t i=0;i<m_thread_count;++i)
	{
		_PoolThread *thrd = new _PoolThread;
		thrd->SetPool(this);
		m_workers.push_back(thrd);
		thrd->Start();
	}
}

ThreadPool::~ThreadPool()
{
	//is it really need to free queue or free poolthreads?
	for(size_t i=0;i<m_workers.size();++i)
	{
		m_workers[i]->Stop();
		PushTask(AutoFunc(NullProc));
	}
	while(m_running_thread_count > 0)
	{
		timespec ts;
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		SleepExactly(&ts);
	}
	delete m_task_queue;
}

BaseFuncClass* ThreadPool::PullTask()
{
	return m_task_queue->C();
}

void ThreadPool::PushTask(BaseFuncClass *task)
{
	m_task_queue->P(task);
}
}
