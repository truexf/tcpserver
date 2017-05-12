/*
 * xlog.h
 *
 *  Created on: Mar 10, 2015
 *      Author: root
 */

#ifndef FYSLIB_XLOG_H_
#define FYSLIB_XLOG_H_

#include "tthread.h"
#include <string.h>
#include <string>
#include <pthread.h>
#include "systemtime.h"

using namespace std;

namespace fyslib{

class XLog: public TThread
{
public:
  XLog(const char *p_logfilenamepath=NULL,int p_logqueue_size = 1024*1024,int p_flush_interval = 1000);
  virtual ~XLog();
  void Log(const void *data,int datasize);
  void LogStr(const char *str);
  void LogWarn(const char *data,bool newLine = true);
  void LogError(const char *data,bool newLine = true);
  void LogInfo(const char *data,bool newLine = true);
  void LogDebug(const char *data,bool newLine = true);
  void SwitchQueue();
protected:
  void Flush();
  void Run(); //override;
private:
  //cannot clone
  XLog(const XLog &);
  XLog& operator=(const XLog &);

  //structures
  void *m_prequeue_logical;
  void *m_savingqueue_logical;
  void *m_prequeue_real;
  void *m_savingqueue_real;
  pthread_mutex_t * m_swaplock;
  pthread_mutex_t * m_prequeuelock;
  int m_prequeue_size;
  int m_savingqueue_size;
  int m_file_handle;
  PSYSTEMTIME m_file_date;
  PSYSTEMTIME m_file_date_tmp;
  char *m_computer_name;

  //parameters
  string m_logfilepath;
  int m_logqueue_size;
  int m_flush_interval;

public:
  char m_logfmt[8192];
  pthread_mutex_t *m_logfmt_lock;
};

extern XLog *_log;
#ifdef _DEBUG
#define XDEBUG
#endif

#ifdef XDEBUG
#define LOGDEBUG(S) _log->LogDebug(S)
#else
#define LOGDEBUG(S)
#endif

const char* FormatLog(XLog *log,const char * str, ...);

#define LOG_ERR(logger,s) logger->LogStr(FormatLog(logger,"ERR  %s %s,%d %s\n",NowStr().c_str(),__FILE__,__LINE__,s));
#define LOG_WARN(logger,s) logger->LogStr(FormatLog(logger,"WARN %s %s,%d %s\n",NowStr().c_str(),__FILE__,__LINE__,s));
#define LOG_INFO(logger,s) logger->LogStr(FormatLog(logger,"INFO %s %s,%d %s\n",NowStr().c_str(),__FILE__,__LINE__,s));

#ifdef XDEBUG
#define LOG_DEBUG(logger,s) logger->LogStr(FormatLog(logger,"DEBUG %s %s,%d %s\n",NowStr().c_str(),__FILE__,__LINE__,s));
#else
#define LOG_DEBUG(logger,s) ;
#endif

} //end of namespace

#endif /* FYSLIB_XLOG_H_ */
