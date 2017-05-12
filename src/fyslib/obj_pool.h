/*
 * object_pool.h
 *
 *  Created on: May 3, 2017
 *      Author: root
 */

#ifndef FYSLIB_OBJ_POOL_H_
#define FYSLIB_OBJ_POOL_H_

#include <set>
using std::set;
#include <pthread.h>

namespace fyslib {

class PoolObject {
public:
    virtual void Init() = 0;
    virtual void Fina() = 0;
    virtual ~PoolObject(){};
};

class ObjPool {
private:
    set<PoolObject*> m_pool;
    pthread_mutex_t *m_pool_lock;
public:
    ObjPool();
    ~ObjPool();
    void Push(PoolObject *obj);
    PoolObject * Pull();
};

}



#endif /* FYSLIB_OBJ_POOL_H_ */
