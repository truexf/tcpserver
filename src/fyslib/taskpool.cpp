/*
 * taskpool.cpp
 *
 *  Created on: Sep 25, 2017
 *      Author: root
 */

#include "taskpool.h"
#include "xtimer.h"

namespace fyslib{

class _TaskThread: public TThread {
private:
    TaskPool *m_pool;
    BaseFuncClass *m_task;
    pthread_cond_t *m_cond;
    pthread_mutex_t *m_mutex;
protected:
    void Run() {
        m_cond = new(pthread_cond_t);
        pthread_cond_init(m_cond,NULL);
        m_mutex = CreateMutex(false);
        for (;;) {
            AutoMutex auto1(m_mutex);
            pthread_cond_wait(m_cond,m_mutex);
            if (GetTerminated())
                break;
            if (m_task) {
                m_task->ExecFunc();
                delete m_task;
                m_task = NULL;
                m_pool->PushThread(this);
            }
        }
        pthread_cond_destroy(m_cond);
        DestroyMutex(m_mutex);
    }
public:
    void SetPool(TaskPool *pool) {
        m_pool = pool;
    }
    void SetTask(BaseFuncClass *task) {
        m_task = task;
    }
    void Notify() {
        pthread_cond_signal(m_cond);
    }
};

TaskPool::TaskPool(size_t priorityNum, size_t taskQueueLen, size_t workThreadCount) {
    sem_init(&m_task_sem,0,0);
    m_task_queues_lock = CreateMutex(false);
    m_work_thread_count = workThreadCount;
    m_queue_len = taskQueueLen;
    m_thread_queue = new Pandc<_TaskThread*>;
    m_thread_queue->SetQueueType(qtFILO);

    for (size_t i = 0; i<priorityNum; i++) {
        LimitedPandc *queue = new LimitedPandc(taskQueueLen, qtFIFO);
        m_task_queues.push_back(queue);
    }
}

TaskPool::~TaskPool() {
    while (m_thread_queue->GetSize() > 0) {
        _TaskThread *t = m_thread_queue->C();
        t->SetFreeOnTerminate(true);
        t->Notify();
    }
    timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    SleepExactly(&ts);
    for (size_t i = 0; i<m_task_queues.size(); i++) {
        delete m_task_queues[i];
    }
    DestroyMutex(m_task_queues_lock);
    sem_destroy(&m_task_sem);
}

void TaskPool::PushThread(_TaskThread *thrd) {
    if (!thrd)
        return;
    m_thread_queue->P(thrd);
}

_TaskThread* TaskPool::PullThread() {
    return m_thread_queue->C();
}

BaseFuncClass* TaskPool::PullTask(const timespec *timeout) {
    bool waited = false;
    if (!timeout) {
        waited = (0 == sem_wait(&m_task_sem));
    } else {
        struct timespec ts;
        if(-1 == clock_gettime(CLOCK_REALTIME,&ts))
            return false;
        ts.tv_sec += timeout->tv_sec;
        ts.tv_nsec += timeout->tv_nsec;
        waited = (0 == sem_timedwait(&m_task_sem,&ts));
    }
    if (!waited)
        return NULL;
    AutoMutex auto1(m_task_queues_lock);
    for (size_t i = 0; i < m_task_queues.size(); i++) {
        if (m_task_queues[i]->GetSize() > 0) {
            return (BaseFuncClass*)(m_task_queues[i]->C());
        }
    }
    return NULL;
}

bool TaskPool::PushTask(size_t priority, BaseFuncClass* task, const timespec *ts) {
    if (priority < 0 || priority > m_task_queues.size() || NULL == task)
        return false;
    return m_task_queues[priority]->TimedP(task,ts);
}

void TaskPool::Run() {
    SetFreeOnTerminate(true);
    for (size_t i=0; i<m_work_thread_count; i++) {
        _TaskThread *thrd = new _TaskThread;
        thrd->SetFreeOnTerminate(true);
        thrd->SetPool(this);
        thrd->Start();
        PushThread(thrd);
    }
    for(;;) {
        timespec ts;
        ts.tv_nsec = 0;
        ts.tv_sec = 1;
        BaseFuncClass *task = PullTask(&ts);
        if (GetTerminated()) {
            break;
        }
        if (task) {
            _TaskThread *thrd = PullThread();
            thrd->SetTask(task);
            thrd->Notify();
        }
    }
}

}

