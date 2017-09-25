/*
 * taskpool.h
 *
 *  Created on: Sep 25, 2017
 *      Author: root
 */

#ifndef FYSLIB_TASKPOOL_H_
#define FYSLIB_TASKPOOL_H_

#include "AutoObject.hpp"
#include "tthread.h"
#include "pandc.hpp"
#include <vector>
#include <deque>
#include <semaphore.h>
using std::vector;
using std::deque;

namespace fyslib{

class _TaskThread;
class TaskPool: public TThread {
    friend class _TaskThread;
private:
    TaskPool(const TaskPool&);
    TaskPool& operator=(const TaskPool&);
public:
    TaskPool(size_t priorityNum, size_t taskQueueLen, size_t workThreadCount);
    virtual ~TaskPool();
private:
    vector<LimitedPandc*> m_task_queues;
    pthread_mutex_t *m_task_queues_lock;
    size_t m_work_thread_count;
    size_t m_queue_len;
    Pandc<_TaskThread*> *m_thread_queue;
    sem_t m_task_sem;

    void PushThread(_TaskThread *thrd);
    _TaskThread* PullThread();
    BaseFuncClass* PullTask(const timespec *timeout);
protected:
    void Run();
public:
    bool PushTask(size_t priority, BaseFuncClass* task, const timespec *ts);
};


}
#endif /* FYSLIB_TASKPOOL_H_ */
