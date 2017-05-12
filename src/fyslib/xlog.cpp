/*
 * xlog.cpp
 *
 *  Created on: Mar 10, 2015
 *      Author: root
 */


#include "xlog.h"
#include "sysutils.h"
#include "tthread.h"
#include <unistd.h>
#include "AutoObject.hpp"
#include "systemtime.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

namespace fyslib{

const char* FormatLog(XLog *log,const char * str, ...)
{
	AutoMutex auto1(log->m_logfmt_lock) ;
	if (str != NULL)
	{
		va_list vl;
		va_start(vl, str);
		int n = vsnprintf(log->m_logfmt, sizeof(log->m_logfmt), str, vl);
		va_end(vl);
		if (n > 0)
		{
			log->m_logfmt[n] = 0;
			return log->m_logfmt;
		}
	}
	return "";
}

XLog *_log = NULL;

typedef unsigned long DWORD;

XLog::XLog(const char *p_logfilenamepath,
           int p_logqueue_size,
           int p_flush_interval)
{
  if (!p_logfilenamepath)
    m_logfilepath.assign(ExtractFilePath(ParamStr(0),true));
  else
  {
    m_logfilepath.assign(p_logfilenamepath);
    if (m_logfilepath.empty() || !DirectoryExists(m_logfilepath.c_str()))
      m_logfilepath.assign(ExtractFilePath(ParamStr(0),true));
    else
      m_logfilepath.assign(IncludeTrailingPathDelimiter(m_logfilepath));
  }
  m_logqueue_size = p_logqueue_size;
  if (m_logqueue_size <= 0)
    m_logqueue_size = 1 * 1024 * 1024; //1M;
  m_flush_interval = p_flush_interval;

  m_swaplock = new pthread_mutex_t();
  pthread_mutex_init(m_swaplock,NULL);
  m_prequeuelock = new pthread_mutex_t();
  pthread_mutex_init(m_prequeuelock,NULL);
  m_prequeue_real = malloc(m_logqueue_size);
  m_savingqueue_real = malloc(m_logqueue_size);
  m_prequeue_logical = m_prequeue_real;
  m_savingqueue_logical = m_savingqueue_real;
  m_prequeue_size = 0;
  m_savingqueue_size = 0;
  m_file_handle = NULL;
  m_file_date = NULL;
  m_file_date_tmp = NULL;
  DWORD dwComputerNameSize = 255;
  m_computer_name = (char*)malloc(dwComputerNameSize*sizeof(char));
  memset(m_computer_name,0,dwComputerNameSize*sizeof(char));
  char s[255];
  memset(s,0,255);
  gethostname(s,255);
  memcpy(m_computer_name,s,strlen(s));
  _log = this;
  memset(m_logfmt,0,sizeof(m_logfmt));
  m_logfmt_lock = CreateMutex(false);
}

XLog::~XLog()
{
  if (m_savingqueue_size > 0)
      Flush();
  if (m_prequeue_size > 0)
  {
      SwitchQueue();
      Flush();
  }
  free(m_computer_name);
  if (m_file_date)
    delete m_file_date;
  if(m_file_date_tmp)
      delete m_file_date_tmp;
  free(m_prequeue_real);
  free(m_savingqueue_real);
  pthread_mutex_destroy(m_prequeuelock);
  delete m_prequeuelock;
  pthread_mutex_destroy(m_swaplock);
  delete m_swaplock;
}


void XLog::Run()
{
  while (!GetTerminated())
  {
    sleep((unsigned int)(m_flush_interval / 1000));
    if (GetTerminated())
      break;
    SwitchQueue();
  }
}

void XLog::Flush()
{
  if (0 == m_savingqueue_size)
    return;
  m_file_date_tmp = new SYSTEMTIME();
  GetLocalTime(m_file_date_tmp);
  string sfile = m_logfilepath;
  sfile.append(m_computer_name);
  sfile.append("_");
  sfile.append(ExtractFileName(ParamStr(0)));
  sfile.append("_");
  sfile.append(FormatDate(*m_file_date_tmp));
  sfile.append(".log");

  if (!FileExists(sfile.c_str()))
  {
//      unsigned char head[2] = {0xFF,0xFE};
      SaveBufferToFile(sfile,m_savingqueue_logical,m_savingqueue_size);
  }
  else
  {
	  AppendBuf2File(sfile,m_savingqueue_logical,m_savingqueue_size);
  }
  m_savingqueue_size = 0;
}

void XLog::Log(const void *data,int datasize )
{
  if (!data || datasize <= 0)
    return;
  AutoMutex auto1(m_prequeuelock);
  if (m_prequeue_size + datasize > m_logqueue_size)
    SwitchQueue();
  void *p = m_prequeue_logical;
  IncPtr(&p,m_prequeue_size);
  memcpy(p,data,datasize);
  m_prequeue_size += datasize;
}

void XLog::SwitchQueue()
{
  AutoMutex auto_0(m_swaplock);
  if (m_savingqueue_size >= 0)
    Flush();
  void *tmp;
  tmp = m_prequeue_logical;
  m_prequeue_logical = m_savingqueue_logical;
  m_savingqueue_logical = tmp;
  m_savingqueue_size = m_prequeue_size;
  m_prequeue_size = 0;
}

void XLog::LogWarn(const  char *data,bool newLine )
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  string s = FormatDatetime(st) + " warn " +  string(data);
  if (newLine)
	  s += "\r\n";
  Log(s.c_str(),s.length()*sizeof(char));
}

void XLog::LogError(const char *data,bool newLine )
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  string s = FormatDatetime(st) + " error " + string(data);
  if (newLine)
	  s += "\r\n";
  Log(s.c_str(),s.length()*sizeof(char));
}

void XLog::LogInfo(const  char *data,bool newLine )
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  string s = FormatDatetime(st) + " info " + string(data);
  if (newLine)
	  s += "\r\n";
  Log(s.c_str(),s.length()*sizeof(char));
}

void XLog::LogDebug(const char *data,bool newLine )
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  string s = FormatDatetime(st) + " debug " + string(data);
  if (newLine)
	  s += "\r\n";
  Log(s.c_str(),s.length()*sizeof(char));
}

void XLog::LogStr(const char *str)
{
	Log(str,strlen(str));
}



}
