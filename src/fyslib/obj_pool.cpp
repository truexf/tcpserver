/*
 * object_pool.cpp
 *
 *  Created on: May 3, 2017
 *      Author: root
 */

#include "obj_pool.h"

#include "AutoObject.hpp"
#include "tthread.h"

namespace fyslib {

ObjPool::ObjPool() {
    m_pool_lock = CreateMutex(false);
}
ObjPool::~ObjPool() {
    DestroyMutex(m_pool_lock);
}
void ObjPool::Push(PoolObject *obj) {
    if (!obj)
        return;
    obj->Fina();
    AutoMutex auto1(m_pool_lock);
    set<PoolObject*>::iterator it = m_pool.find(obj);
    if (it == m_pool.end()) {
        m_pool.insert(obj);
    }
}
PoolObject * ObjPool::Pull() {
    AutoMutex auto1(m_pool_lock);
    if (m_pool.empty())
        return NULL;
    PoolObject *ret = *m_pool.rbegin();
    m_pool.erase(ret);
    ret->Init();
    return ret;
}


}



