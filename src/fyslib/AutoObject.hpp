/*
 * AutoObject.hpp
 *
 *  Created on: Jan 23, 2015
 *      Author: root
 */

#ifndef FYSLIB_AUTOOBJECT_HPP_
#define FYSLIB_AUTOOBJECT_HPP_

#include <pthread.h>

namespace fyslib{

class AutoMutex
{
public:
	pthread_mutex_t *f_mutex;
	AutoMutex(pthread_mutex_t *mutex){
		f_mutex = mutex;
		pthread_mutex_lock(f_mutex);
	}
	~AutoMutex(){
		pthread_mutex_unlock(f_mutex);
	}
private:
	AutoMutex(const AutoMutex&);
	AutoMutex& operator = (const AutoMutex&);
};

class BaseFuncClass
{
public:
  BaseFuncClass(){};
  virtual ~BaseFuncClass(){};
  virtual void ExecFunc(){};
private:
  BaseFuncClass(const BaseFuncClass&);
  BaseFuncClass& operator=(const BaseFuncClass&);
};

template<typename OBJTYPE,typename FUNCTYPE>
class ObjFuncClass0: public BaseFuncClass
{
public:
  ObjFuncClass0(OBJTYPE *obj,FUNCTYPE func){m_func = func;m_obj = obj;}
  void ExecFunc(){if (m_func && m_obj) (m_obj->*m_func)();}
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1>
class ObjFuncClass1: public BaseFuncClass
{
public:
  ObjFuncClass1(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1){m_func = func;m_obj = obj; m_arg1 = arg1;}
  void ExecFunc()
  {
	  if (m_func && m_obj)
		  (m_obj->*m_func)(m_arg1);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2>
class ObjFuncClass2: public BaseFuncClass
{
public:
  ObjFuncClass2(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3>
class ObjFuncClass3: public BaseFuncClass
{
public:
  ObjFuncClass3(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4
>
class ObjFuncClass4: public BaseFuncClass
{
public:
  ObjFuncClass4(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5
>
class ObjFuncClass5: public BaseFuncClass
{
public:
  ObjFuncClass5(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
>
class ObjFuncClass6: public BaseFuncClass
{
public:
  ObjFuncClass6(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
    ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7
>
class ObjFuncClass7: public BaseFuncClass
{
public:
  ObjFuncClass7(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
    ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7,typename ARGTYPE8
>
class ObjFuncClass8: public BaseFuncClass
{
public:
  ObjFuncClass8(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
    ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7,ARGTYPE8 arg8)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
    m_arg8 = arg8;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7,m_arg8);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
  ARGTYPE8 m_arg8;
};

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7,typename ARGTYPE8,typename ARGTYPE9
>
class ObjFuncClass9: public BaseFuncClass
{
public:
  ObjFuncClass9(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
    ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7,ARGTYPE8 arg8,ARGTYPE9 arg9)
  {
    m_func = func;
    m_obj = obj;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
    m_arg8 = arg8;
    m_arg9 = arg9;
  }
  void ExecFunc()
  {
	    if (m_func && m_obj)
	      (m_obj->*m_func)(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7,m_arg8,m_arg9);
  }
private:
  FUNCTYPE m_func;
  OBJTYPE *m_obj;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
  ARGTYPE8 m_arg8;
  ARGTYPE9 m_arg9;
};

template<typename OBJTYPE,typename FUNCTYPE>
ObjFuncClass0<OBJTYPE,FUNCTYPE> *ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func)
{
  return new ObjFuncClass0<OBJTYPE,FUNCTYPE>(obj,func);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1>
ObjFuncClass1<OBJTYPE,FUNCTYPE,ARGTYPE1> *ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1)
{
  return new ObjFuncClass1<OBJTYPE,FUNCTYPE,ARGTYPE1>(obj,func,arg1);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2>
ObjFuncClass2<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2> *ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2)
{
  return new ObjFuncClass2<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2>(obj,func,arg1,arg2);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3>
ObjFuncClass3<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3)
{
  return new ObjFuncClass3<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3>(obj,func,arg1,arg2,arg3);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
  ,typename ARGTYPE4>
ObjFuncClass4<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4)
{
  return new ObjFuncClass4<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4>(obj,func,arg1,arg2,arg3,arg4);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5
>
ObjFuncClass5<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4,ARGTYPE5 arg5
             )
{
  return new ObjFuncClass5<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5>(obj,func,arg1,arg2,arg3
    ,arg4,arg5);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
>
ObjFuncClass6<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
             )
{
  return new ObjFuncClass6<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6>
    (obj,func,arg1,arg2,arg3
    ,arg4,arg5,arg6);
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7
>
ObjFuncClass7<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
,ARGTYPE7>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
             ,ARGTYPE4 arg7
             )
{
  return new ObjFuncClass7<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
    ,ARGTYPE7>
    (obj,func,arg1,arg2,arg3
    ,arg4,arg5,arg6
    ,arg7
    );
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7,typename ARGTYPE8
>
ObjFuncClass8<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
,ARGTYPE7,ARGTYPE8>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
             ,ARGTYPE4 arg7,ARGTYPE8 arg8
             )
{
  return new ObjFuncClass8<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
    ,ARGTYPE7,ARGTYPE8>
    (obj,func,arg1,arg2,arg3
    ,arg4,arg5,arg6
    ,arg7,arg8
    );
}

template<typename OBJTYPE,typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3
,typename ARGTYPE4,typename ARGTYPE5,typename ARGTYPE6
,typename ARGTYPE7,typename ARGTYPE8,typename ARGTYPE9
>
ObjFuncClass9<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
,ARGTYPE7,ARGTYPE8,ARGTYPE9>
*ObjAutoFunc(OBJTYPE *obj,FUNCTYPE func
             ,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3
             ,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
             ,ARGTYPE4 arg7,ARGTYPE8 arg8,ARGTYPE9 arg9
             )
{
  return new ObjFuncClass9<OBJTYPE,FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6
  ,ARGTYPE7,ARGTYPE8,ARGTYPE9>
    (obj,func,arg1,arg2,arg3
    ,arg4,arg5,arg6
    ,arg7,arg8,arg9
    );
}


template<typename FUNCTYPE>
class FuncClass0: public BaseFuncClass
{
public:
  FuncClass0(FUNCTYPE func){m_func = func;}
  void ExecFunc(){if (m_func) m_func();}
private:
  FUNCTYPE m_func;
};


template<typename FUNCTYPE,typename ARGTYPE1>
class FuncClass1: public BaseFuncClass
{
public:
  FuncClass1(FUNCTYPE func,ARGTYPE1 arg1)
  {
    m_func = func;
    m_arg1 = arg1;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;

};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2>
class FuncClass2: public BaseFuncClass
{
public:
  FuncClass2(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3>
class FuncClass3: public BaseFuncClass
{
public:
  FuncClass3(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4>
class FuncClass4: public BaseFuncClass
{
public:
  FuncClass4(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4,typename ARGTYPE5>
class FuncClass5: public BaseFuncClass
{
public:
  FuncClass5(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4,typename ARGTYPE5
,typename ARGTYPE6>
class FuncClass6: public BaseFuncClass
{
public:
  FuncClass6(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4,typename ARGTYPE5
,typename ARGTYPE6,typename ARGTYPE7>
class FuncClass7: public BaseFuncClass
{
public:
  FuncClass7(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4,typename ARGTYPE5
,typename ARGTYPE6,typename ARGTYPE7,typename ARGTYPE8>
class FuncClass8: public BaseFuncClass
{
public:
  FuncClass8(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7,ARGTYPE8 arg8)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
    m_arg8 = arg8;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7,m_arg8);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
  ARGTYPE8 m_arg8;
};

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4,typename ARGTYPE5
,typename ARGTYPE6,typename ARGTYPE7,typename ARGTYPE8,typename ARGTYPE9>
class FuncClass9: public BaseFuncClass
{
public:
  FuncClass9(FUNCTYPE func,ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6
    ,ARGTYPE7 arg7,ARGTYPE8 arg8,ARGTYPE9 arg9)
  {
    m_func = func;
    m_arg1 = arg1;
    m_arg2 = arg2;
    m_arg3 = arg3;
    m_arg4 = arg4;
    m_arg5 = arg5;
    m_arg6 = arg6;
    m_arg7 = arg7;
    m_arg8 = arg8;
    m_arg9 = arg9;
  }
  void ExecFunc()
  {
	  if (m_func) m_func(m_arg1,m_arg2,m_arg3,m_arg4,m_arg5,m_arg6,m_arg7,m_arg8,m_arg9);
  }
private:
  FUNCTYPE m_func;
  ARGTYPE1 m_arg1;
  ARGTYPE2 m_arg2;
  ARGTYPE3 m_arg3;
  ARGTYPE4 m_arg4;
  ARGTYPE5 m_arg5;
  ARGTYPE6 m_arg6;
  ARGTYPE7 m_arg7;
  ARGTYPE8 m_arg8;
  ARGTYPE9 m_arg9;
};

//...3,4,5,6,7,8,9

template<typename FUNCTYPE>
FuncClass0<FUNCTYPE> *AutoFunc(FUNCTYPE func_ptr)
{
  FuncClass0<FUNCTYPE> *ptr = new FuncClass0<FUNCTYPE>(func_ptr);
  return ptr;
}

template<typename FUNCTYPE,typename ARGTYPE1>
FuncClass1<FUNCTYPE,ARGTYPE1> *AutoFunc(FUNCTYPE func_ptr,ARGTYPE1 arg1)
{
  return (new FuncClass1<FUNCTYPE,ARGTYPE1>(func_ptr,arg1));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2>
FuncClass2<FUNCTYPE,ARGTYPE1,ARGTYPE2> *AutoFunc(FUNCTYPE func_ptr,ARGTYPE1 arg1,ARGTYPE2 arg2)
{
  return (new FuncClass2<FUNCTYPE,ARGTYPE1,ARGTYPE2>(func_ptr,arg1,arg2));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3>
FuncClass3<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3>
*AutoFunc(FUNCTYPE func_ptr,
          ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3)
{
  return (new FuncClass3<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3>(func_ptr,arg1,arg2,arg3));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4>
FuncClass4<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4>
*AutoFunc(FUNCTYPE func_ptr,
          ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
          ARGTYPE4 arg4)
{
  return (new FuncClass4<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4>(func_ptr,arg1,arg2,arg3,arg4));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4
,typename ARGTYPE5>
FuncClass5<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,
ARGTYPE5> *AutoFunc(FUNCTYPE func_ptr,
                    ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
                    ARGTYPE4 arg4,ARGTYPE5 arg5)
{
  return (new FuncClass5<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5>(func_ptr,arg1,arg2,arg3,arg4,arg5));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4
,typename ARGTYPE5,typename ARGTYPE6>
FuncClass6<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,
ARGTYPE5,ARGTYPE6> *AutoFunc(FUNCTYPE func_ptr,
                             ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
                             ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6)
{
  return (new FuncClass6<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6>
    (func_ptr,arg1,arg2,arg3,arg4,arg5,arg6));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4
,typename ARGTYPE5,typename ARGTYPE6,typename ARGTYPE7>
FuncClass7<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,
ARGTYPE5,ARGTYPE6,ARGTYPE7> *AutoFunc(FUNCTYPE func_ptr,
                                      ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
                                      ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6,
                                      ARGTYPE7 arg7)
{
  return (new FuncClass7<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6,ARGTYPE7>
    (func_ptr,arg1,arg2,arg3,arg4,arg5,arg6,arg7));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4
,typename ARGTYPE5,typename ARGTYPE6,typename ARGTYPE7,typename ARGTYPE8>
FuncClass8<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,
ARGTYPE5,ARGTYPE6,ARGTYPE7,ARGTYPE8> *AutoFunc(FUNCTYPE func_ptr,
                                               ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
                                               ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6,
                                               ARGTYPE7 arg7,ARGTYPE8 arg8)
{
  return (new FuncClass8<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6,ARGTYPE7,ARGTYPE8>
    (func_ptr,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8));
}

template<typename FUNCTYPE,typename ARGTYPE1,typename ARGTYPE2,typename ARGTYPE3,typename ARGTYPE4
,typename ARGTYPE5,typename ARGTYPE6,typename ARGTYPE7,typename ARGTYPE8,typename ARGTYPE9>
FuncClass9<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,
ARGTYPE5,ARGTYPE6,ARGTYPE7,ARGTYPE8,ARGTYPE9> *AutoFunc(FUNCTYPE func_ptr,
                                                        ARGTYPE1 arg1,ARGTYPE2 arg2,ARGTYPE3 arg3,
                                                        ARGTYPE4 arg4,ARGTYPE5 arg5,ARGTYPE6 arg6,
                                                        ARGTYPE7 arg7,ARGTYPE8 arg8,ARGTYPE9 arg9)
{
  return (new FuncClass9<FUNCTYPE,ARGTYPE1,ARGTYPE2,ARGTYPE3,ARGTYPE4,ARGTYPE5,ARGTYPE6,ARGTYPE7,ARGTYPE8,ARGTYPE9>
    (func_ptr,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9));
}

class AutoFuncClass
{
public:
  AutoFuncClass(BaseFuncClass *obj):m_funcobj(obj){}
  ~AutoFuncClass()
  {
	  if (m_funcobj){
		  m_funcobj->ExecFunc();
		  delete m_funcobj;
	  }
  }
private:
  BaseFuncClass *m_funcobj;
};

//...3,4,5,6,7,8,9

//使用方式
/*
int myfunc();
int myfunc1(string arg)；

void myfunc()
{
AutoClass c1(AutoFunc(myfunc));
AutoClass c2(AutoFunc(myfunc1,string("hello")));

在作用域结束的时候会调用上面两个AutoFunc返回的对象的析构，从而调用了myfunc和myfunc1;
}

*/

}//end of namespace fyslib

#endif /* FYSLIB_AUTOOBJECT_HPP_ */
