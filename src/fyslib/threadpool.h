/*
 * threadpool.h
 *
 *  Created on: Mar 19, 2015
 *      Author: root
 */

#ifndef FYSLIB_THREADPOOL_H_
#define FYSLIB_THREADPOOL_H_

#include "AutoObject.hpp"
#include "tthread.h"
#include "pandc.hpp"
#include <vector>

using std::vector;

namespace fyslib{

class _PoolThread;
class ThreadPool
{
	friend class _PoolThread;
public:
	ThreadPool(int thread_count);
	virtual ~ThreadPool();
	void PushTask(BaseFuncClass *task);
private:
	BaseFuncClass* PullTask();
	void IncRunningThreadCount()
	{
		__sync_add_and_fetch(&m_running_thread_count,1);
	}
	void DecRunningThreadCount()
	{
		__sync_sub_and_fetch(&m_running_thread_count,1);
	}
private:
	ThreadPool(const ThreadPool&);
	ThreadPool& operator = (const ThreadPool&);

	size_t m_thread_count;
	volatile int m_running_thread_count;
	Pandc<BaseFuncClass*> *m_task_queue;
	vector<_PoolThread*> m_workers;
};

}
#endif /* FYSLIB_THREADPOOL_H_ */
