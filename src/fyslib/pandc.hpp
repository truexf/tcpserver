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
using std::deque;
using std::vector;

namespace fyslib
{

template<typename T>
class Pandc
{
public:
	Pandc()
	{
		m_queue_lock = CreateMutex(false);
		sem_init(&m_sem, 0, 0);
	}
	~Pandc()
	{
		sem_destroy(&m_sem);
		DestroyMutex(m_queue_lock);
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
		sem_wait(&m_sem);
		AutoMutex auto1(m_queue_lock);
		T ret = m_queue.front();
		m_queue.pop_front();
		return ret;
	}
	bool TimedC(const struct timespec *timeout, T &ret)
	{
		bool waited;
		if (NULL == timeout || (0 == timeout->tv_nsec && 0 == timeout->tv_sec))
			waited = (0 == sem_trywait(&m_sem));
		else
			waited = (0 == sem_timedwait(&m_sem, timeout));
		if (!waited)
			return false;
		AutoMutex auto1(m_queue_lock);
		ret = m_queue.front();
		m_queue.pop_front();
		return true;
	}
private:
	deque<T> m_queue;
	pthread_mutex_t *m_queue_lock;
	sem_t m_sem;
};


class LimitedPandc
{
public:
	LimitedPandc(int queue_len)
	{
		m_queue_lock = CreateMutex(false);
		sem_init(&m_sem, 0, 0);
		sem_init(&m_sem_free,0,queue_len);
		m_queue_len = queue_len;
		m_head = -1;
		m_tail = -1;
		m_queue = new void*[queue_len];
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
		sem_wait(&m_sem_free);
		{
			AutoMutex auto1(m_queue_lock);
			++m_head;
			if(m_head == m_queue_len)
				m_head = 0;
			m_queue[m_head] = p;
		}
		sem_post(&m_sem);
	}
	bool TimedP(void *p,const struct timespec *timeout)
	{
		bool waited;
		if(NULL == timeout)
			waited = (0 == sem_trywait(&m_sem_free));
		else
		{
			struct timespec ts;
			if(-1 == clock_gettime(CLOCK_REALTIME,&ts))
				return false;
			ts.tv_sec += timeout->tv_sec;
			ts.tv_nsec += timeout->tv_nsec;
			waited = (0 == sem_timedwait(&m_sem_free,&ts));
		}
		if(!waited)
			return false;
		{
			AutoMutex auto1(m_queue_lock);
			++m_head;
			if(m_head == m_queue_len)
				m_head = 0;
			m_queue[m_head] = p;
		}
		sem_post(&m_sem);
	}
	void* C()
	{
		void *ret = NULL;
		sem_wait(&m_sem);
		{
			AutoMutex auto1(m_queue_lock);
			++m_tail;
			if(m_tail == m_queue_len)
				m_tail = 0;
			ret = m_queue[m_tail];
		}
		sem_post(&m_sem_free);
		return ret;
	}
	bool TimedC(const struct timespec *timeout,void **ret)
	{
		bool waited;
		if(NULL == timeout)
			waited = (0 == sem_trywait(&m_sem));
		else
		{
			struct timespec ts;
			if(-1 == clock_gettime(CLOCK_REALTIME,&ts))
				return false;
			ts.tv_sec += timeout->tv_sec;
			ts.tv_nsec += timeout->tv_nsec;
			waited = (0 == sem_timedwait(&m_sem,&ts));
		}
		if(!waited)
			return false;
		{
			AutoMutex auto1(m_queue_lock);
			++m_tail;
			if(m_tail == m_queue_len)
				m_tail = 0;
			*ret = m_queue[m_tail];
		}
		sem_post(&m_sem_free);
		return true;
	}
private:
	void** m_queue;
	pthread_mutex_t *m_queue_lock;
	sem_t m_sem;
	sem_t m_sem_free;
	int m_head,m_tail,m_queue_len;

};

} //end of namespace
#endif /* FYSLIB_PANDC_HPP_ */
