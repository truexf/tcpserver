/*
 * pandc.hpp 线程安全的生产者/消费者队列
 *
 *  Created on: Mar 11, 2015
 *      Author: fangyousong@qq.com
 */

#ifndef FYSLIB_PANDC_HPP_
#define FYSLIB_PANDC_HPP_

#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include "tthread.h"
#include <stdlib.h>
#include <string.h>
#include <deque>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <errno.h>
using std::deque;
using std::vector;

namespace fyslib
{
enum QueueType
{
  qtFIFO,
  qtFILO
};
template<typename T>
class Pandc
{
public:
	Pandc()
	{
		m_queue_lock = CreateMutex(true);
		sem_init(&m_sem, 0, 0);
		SetQueueType(qtFIFO);
	}
	~Pandc()
	{
		sem_destroy(&m_sem);
		DestroyMutex(m_queue_lock);
	}
	void SetQueueType(const QueueType tp) {
	    m_queue_type = tp;
	}
	void Lock() {
	    LockMutex(m_queue_lock);
	}
	void Unlock() {
	    UnlockMutex(m_queue_lock);
	}
	void P(T p)
	{
		{
			AutoMutex auto1(m_queue_lock);
		    m_queue.push_back(p);
		}
		sem_post(&m_sem);
	}
	T C()
	{
	    for(;;) {
	        int ret = sem_wait(&m_sem);
	        if (0 == ret)
	            break;
		}
		AutoMutex auto1(m_queue_lock);
		if (qtFIFO == m_queue_type) {
		    T ret = m_queue.front();
		    m_queue.pop_front();
		    return ret;
		} else {
            T ret = m_queue.back();
            m_queue.pop_back();
            return ret;
		}
	}
	bool TimedC(const struct timespec *timeout, T &ret)
	{
		bool waited;
		if(NULL == timeout || timeout->tv_nsec < 0 || timeout->tv_sec < 0 || timeout->tv_nsec > 1000000000)
			waited = (0 == sem_trywait(&m_sem));
		else {
		    timespec ts;
		    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
		        return false;
		    ts.tv_nsec += timeout->tv_nsec;
		    if (ts.tv_nsec > 1000000000) {
		        ts.tv_nsec -= 1000000000;
		        ts.tv_sec += 1;
		    }
		    ts.tv_sec += timeout->tv_sec;
		    for(;;) {
		        if (0 == sem_timedwait(&m_sem, &ts)) {
		            waited = true;
		            break;
		        } else {
		            if (ETIMEDOUT == errno) {
		                waited = false;
		                break;
		            }
		        }
		    }
		}
		if (!waited)
			return false;
		AutoMutex auto1(m_queue_lock);
		if (qtFIFO == m_queue_type) {
            ret = m_queue.front();
            m_queue.pop_front();
		} else {
            ret = m_queue.back();
            m_queue.pop_back();
		}
        return true;
	}
	size_t GetSize() {
	    return m_queue.size();
	}
private:
	deque<T> m_queue;
	pthread_mutex_t *m_queue_lock;
	sem_t m_sem;
	QueueType m_queue_type;
};


class LimitedPandc
{
public:
	LimitedPandc(int queue_len, QueueType qt)
	{
		m_queue_lock = CreateMutex(false);
		sem_init(&m_sem, 0, 0);
		sem_init(&m_sem_free,0,queue_len);
		m_queue_len = queue_len;
		m_head = -1;
		m_tail = -1;
		m_queue = new void*[queue_len];
		m_size = 0;
		m_queue_type = qt;
	}
	~LimitedPandc()
	{
		sem_destroy(&m_sem_free);
		sem_destroy(&m_sem);
		DestroyMutex(m_queue_lock);
		delete [] m_queue;
	}
	void P(void* p)
	{
	    for(;;) {
	        if (0 == sem_wait(&m_sem_free))
	                break;
	    }
		{
			AutoMutex auto1(m_queue_lock);
			++m_head;
			if(m_head == m_queue_len)
				m_head = 0;
			m_queue[m_head] = p;
			m_size++;
		}
		sem_post(&m_sem);
	}
	bool TimedP(void *p,const struct timespec *timeout)
	{
		bool waited;
		if(NULL == timeout || timeout->tv_nsec < 0 || timeout->tv_sec < 0 || timeout->tv_nsec > 1000000000)
			waited = (0 == sem_trywait(&m_sem_free));
		else
		{
			struct timespec ts;
			if(-1 == clock_gettime(CLOCK_REALTIME,&ts))
				return false;
			ts.tv_sec += timeout->tv_sec;
			ts.tv_nsec += timeout->tv_nsec;
		    if (ts.tv_nsec > 1000000000) {
		        ts.tv_nsec -= 1000000000;
		        ts.tv_sec += 1;
		    }
		    for(;;) {
		        if (0 == sem_timedwait(&m_sem_free,&ts)) {
		            waited = true;
		            break;
		        }
		        if (ETIMEDOUT == errno) {
		            waited = false;
		            break;
		        }
		    }
		}
		if(!waited)
			return false;
		{
			AutoMutex auto1(m_queue_lock);
			++m_head;
			if(m_head == m_queue_len)
				m_head = 0;
			m_queue[m_head] = p;
			m_size++;
		}
		sem_post(&m_sem);
		return true;
	}
	void* C()
	{
		void *ret = NULL;
		sem_wait(&m_sem);
		{
			AutoMutex auto1(m_queue_lock);
            if (qtFIFO == m_queue_type) {
                ++m_tail;
                if(m_tail == m_queue_len)
                    m_tail = 0;
                ret = m_queue[m_tail];
            } else {
                ret = m_queue[m_head];
                m_head--;
            }
			m_size--;
		}
		sem_post(&m_sem_free);
		return ret;
	}
	bool TimedC(const struct timespec *timeout,void **ret)
	{
		bool waited;
		if(NULL == timeout || timeout->tv_nsec < 0 || timeout->tv_sec < 0 || timeout->tv_nsec > 1000000000)
			waited = (0 == sem_trywait(&m_sem));
		else
		{
			struct timespec ts;
			if(-1 == clock_gettime(CLOCK_REALTIME,&ts))
				return false;
			ts.tv_sec += timeout->tv_sec;
			ts.tv_nsec += timeout->tv_nsec;
            if (ts.tv_nsec > 1000000000) {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec += 1;
            }
            for(;;) {
                waited = (0 == sem_timedwait(&m_sem,&ts));
                if (waited)
                    break;
                if (ETIMEDOUT == errno) {
                    break;
                }
            }
		}
		if(!waited)
			return false;
		{
			AutoMutex auto1(m_queue_lock);
			if (qtFIFO == m_queue_type) {
                ++m_tail;
                if(m_tail == m_queue_len)
                    m_tail = 0;
                *ret = m_queue[m_tail];
			} else {
			    *ret = m_queue[m_head];
			    m_head--;
			}
			m_size--;
		}
		sem_post(&m_sem_free);
		return true;
	}
	size_t GetSize() {
	    return m_size;
	}
private:
	void** m_queue;
	pthread_mutex_t *m_queue_lock;
	sem_t m_sem;
	sem_t m_sem_free;
	int m_head,m_tail,m_queue_len;
	size_t m_size;
	QueueType m_queue_type;
};

} //end of namespace
#endif /* FYSLIB_PANDC_HPP_ */
