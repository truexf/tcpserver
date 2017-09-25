/*
 * xtimer.h
 *
 *  Created on: Mar 11, 2015
 *      Author: root
 */

#ifndef FYSLIB_XTIMER_H_
#define FYSLIB_XTIMER_H_

#include <stdlib.h>
#include <time.h>
#include "tthread.h"
#include "AutoObject.hpp"

namespace fyslib{

int SleepExactly(const struct timespec *t);

TThread *StartAsyncTimer(BaseFuncClass *proc,bool runfirst,const struct timespec *t);
void AsyncExecute(BaseFuncClass *proc);
inline void StopAsyncTimer(TThread *thrd)
{
	thrd->Stop();
}

int DoTimeout(BaseFuncClass *proc, const struct timespec *timeout);

}

#endif /* FYSLIB_XTIMER_H_ */
