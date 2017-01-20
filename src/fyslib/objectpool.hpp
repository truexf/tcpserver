/*
 * objectpool.hpp
 *
 *  Created on: Apr 3, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_
#define HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_

#include <vector>
#include <limits.h>
#include "AutoObject.hpp"
#include "tthread.h"

using std::vector;

namespace fyslib{

template<typename T>
class ObjectPool
{
private:
	vector<T*> m_pool;
	pthread_mutex_t *m_lock;
	int m_capacity;
	long long  m_newCount;
public:
	ObjectPool()
	{
		m_lock = CreateMutex(false);
		m_capacity = 1024;
		m_newCount = 0;
	}
	~ObjectPool()
	{
		{
			AutoMutex auto1(m_lock);
			for(size_t i=0;i<m_pool.size();++i)
			{
				delete (T*)(m_pool[i]);
			}
			m_pool.clear();
		}
		DestroyMutex(m_lock);
	}
	void SetCapacity(int v)
	{
		if (v > 0)
			m_capacity = v;
		else
			m_capacity = INT_MAX;
	}
	void PushObject(T* obj)
	{
		AutoMutex auto1(m_lock);
		if(m_pool.size() < (size_t)m_capacity)
			m_pool.push_back(obj);
		else
			delete obj;
	}
	T* PullObject()
	{
		AutoMutex auto1(m_lock);
		if(m_pool.empty()){
			return NULL;
		}
		T* ret = m_pool.back();
		m_pool.pop_back();
		return ret;
	}
	size_t Size()
	{
		AutoMutex auto1(m_lock);
		return m_pool.size();
	}
	long long NewCount()
	{
		AutoMutex auto1(m_lock);
		return m_newCount;
	}
//	void RemoveObject(T *obj)
//	{
//		AutoMutex auto1(m_lock);
//		for (vector<T*>::iterator it = m_pool.begin(); it != m_pool.end(); it++)
//		{
//			if (*it == obj)
//			{
//				m_pool.erase(it);
//				break;
//			}
//		}
//	}
	void SeekAllObjects(vector<T*> &ret)
	{
		AutoMutex auto1(m_lock);
		ret.assign(m_pool.begin(),m_pool.end());
	}
private:
	ObjectPool(const ObjectPool&);
	ObjectPool& operator=(const ObjectPool&);
};


}//endof namespace

#endif /* HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_ */
