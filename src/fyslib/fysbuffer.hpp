/*
 * fysbuffer.hpp
 *
 *  Created on: Mar 26, 2015
 *      Author: root
 */

#ifndef FYSLIB_FYSBUFFER_HPP_
#define FYSLIB_FYSBUFFER_HPP_

/*
                无锁内存缓冲池
其特点如下:
池内存总大小和池内内存块的大小是固定长度的，通过构造函数的参数指定；
分配出来的池内存是经过memset确认的物理内存；
池大小是固定的,若池中空闲内存不足以提供所需，则池内存分配函数利用malloc向系统额外申请，但新申请的内存不并入池中；
通过GetBuf()和FreeBuf两个函数进程池内存的申请和释放；
*/

#include <deque>
#include <algorithm>
#include "tthread.h"
#include "sysutils.h"
using std::deque;

namespace fyslib{
class HBuff
{
public:
    HBuff(long buf_count,long buf_size)
        :m_buf_count(buf_count),m_buf_size(buf_size)
    {
        m_lock = CreateMutex(false);
        m_buf = malloc(buf_count * buf_size);
        memset(m_buf,0,buf_count * buf_size);
        m_buf_end = (POINTER)m_buf + (buf_count *  buf_size);
        m_buf_free = new void*[buf_count];
        void *p = m_buf;
        for(int i=0;i<buf_count;++i)
        {
            m_buf_free[i] = p;
            IncPtr(&p,buf_size);
        }
        m_buf_free_pos = buf_count - 1;
    }
    ~HBuff()
    {
        ::free(m_buf);
        delete [] m_buf_free;
        DestroyMutex(m_lock);
        delete m_lock;
    }
    void *GetBuf()
    {
        AutoMutex auto1(m_lock);
        --m_buf_free_pos;
        if(m_buf_free_pos < -1)
        {
            ++m_buf_free_pos;
            void *ret = malloc(m_buf_size);
            memset(ret,0,m_buf_size);
            m_buf_busy.push_back(ret);
            return ret;
        }
        else
        {
            void* ret = m_buf_free[m_buf_free_pos+1];
            m_buf_busy.push_back(ret);
            return ret;
        }
    }
    void FreeBuf(void *buf)
    {
        AutoMutex auto1(*m_lock);
        deque<void*>::iterator it = std::find(m_buf_busy.begin(),m_buf_busy.end(),buf);
        if(m_buf_busy.end() == it)
            return;
        m_buf_busy.erase(it);
        if((POINTER)buf >= m_buf_end || (POINTER)buf < (POINTER)m_buf)
        {
            ::free(buf);
        }
        else
        {
            ++m_buf_free_pos;
            m_buf_free[m_buf_free_pos] = buf;
        }
    }
private:
    HBuff(const HBuff&);
    HBuff& operator=(const HBuff&);
private:
    long    m_buf_count;
    long    m_buf_size;
    POINTER m_buf_end;
    void*   m_buf;
    void**  m_buf_free;
    deque<void*>  m_buf_busy;
    long volatile m_buf_free_pos;
    pthread_mutex_t *m_lock;
};

}//endof namespace



#endif /* FYSLIB_FYSBUFFER_HPP_ */
