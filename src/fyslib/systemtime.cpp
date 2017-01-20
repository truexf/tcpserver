#include "systemtime.h"
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

namespace fyslib{

void GetLocalTime(SYSTEMTIME *st)
{
	if (NULL == st)
		return;
	struct timeval tv;
	memset(&tv,0,sizeof(tv));
	gettimeofday(&tv,NULL);
	tm t = *localtime(&tv.tv_sec);
	st->wYear = t.tm_year + 1900;
	st->wMonth = t.tm_mon + 1;
	st->wDay = t.tm_mday;
	st->wHour = t.tm_hour;
	st->wMinute = t.tm_min;
	st->wSecond = t.tm_sec;
	st->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}

string FormatDatetime(const SYSTEMTIME &st)
{
  char cst[30]={0};
  sprintf(cst,"%4d-%02d-%02d %02d:%02d:%02d:%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
  return string(cst);
}

string NowStr()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return FormatDatetime(st);
}

string FormatDate(const SYSTEMTIME &st)
{
  char cst[30]={0};
  sprintf(cst,"%4d-%02d-%02d",st.wYear,st.wMonth,st.wDay);
  return string(cst);
}

string FormatTime(const SYSTEMTIME &st)
{
  char cst[30]={0};
  sprintf(cst,"%02d:%02d:%02d:%03d",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
  return string(cst);
}

std::wstring FormatDatetimeW( const SYSTEMTIME &st )
{
  wchar_t cst[30]={0};
  swprintf(cst,30,L"%4d-%02d-%02d %02d:%02d:%02d:%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
  return wstring(cst);
}

std::wstring FormatDateW( const SYSTEMTIME &st )
{
  wchar_t cst[30]={0};
  swprintf(cst,30,L"%4d-%02d-%02d",st.wYear,st.wMonth,st.wDay);
  return wstring(cst);
}

std::wstring FormatTimeW( const SYSTEMTIME &st )
{
  wchar_t cst[30]={0};
  swprintf(cst,30,L"%02d:%02d:%02d:%03d",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
  return wstring(cst);
}

bool Str2Datetime( const string &str, SYSTEMTIME &ret )
{
  memset(&ret,0,sizeof(SYSTEMTIME));
  //YYYY-MM-DD HH:NN:SS:ZZZ
  if (str.length() < strlen("YYYY-MM-DD"))
    return false;
  string sY,sM,sD,sH,sN,sS,sZ;
  int iY=0,iM=0,iD=0,iH=0,iN=0,iS=0,iZ=0;
  sY = str.substr(0,4);
  iY = atoi(sY.c_str());
  if (0==iY)
    return false;
  sM = str.substr(5,2);
  iM = atoi(sM.c_str());
  if (0==iM || 12<iM)
    return false;
  sD = str.substr(8,2);
  iD = atoi(sD.c_str());
  if (0==iD || 31<iD)
    return false;
  if (str.length()>=13)
    sH = str.substr(11,2);
  if (str.length()>=16)
    sN = str.substr(14,2);
  if (str.length()>=19)
    sS = str.substr(17,2);
  if (str.length()>=23)
    sZ = str.substr(20,3);
  iH = atoi(sH.c_str());
  if (iH>23)
    iH = 0;
  iN = atoi(sN.c_str());
  if (iN>59)
    iN = 0;
  iS = atoi(sS.c_str());
  if (iS>59)
    iS = 0;
  iZ = atoi(sZ.c_str());
  if (iZ > 999)
    iZ = 0;

  ret.wYear = (WORD)iY;
  ret.wMonth = (WORD)iM;
  ret.wDay = (WORD)iD;
  ret.wHour = (WORD)iH;
  ret.wMinute = (WORD)iN;
  ret.wSecond = (WORD)iS;
  ret.wMilliseconds = (WORD)iZ;

  return true;
}

inline bool IsValidateDate( SYSTEMTIME &dt )
{
 if (dt.wMonth > 12 || dt.wMonth == 0
   || dt.wDay > 31 || dt.wDay == 0
   || (dt.wMonth == 2 && IsLeapYear(dt.wYear) && dt.wDay>29)
   || (dt.wMonth == 2 && (!IsLeapYear(dt.wYear)) && dt.wDay>28)
   || ((dt.wMonth == 4 || dt.wMonth == 6 || dt.wMonth == 9 || dt.wMonth == 11) && dt.wDay > 30)
   )
   return false;
 return true;
}

bool IsValidateDateTime( SYSTEMTIME &dt )
{
  if (!IsValidateDate(dt))
    return false;
  return (dt.wHour >= 0 && dt.wHour<24 && dt.wMinute>=0 && dt.wMinute<60 && dt.wSecond>=0 && dt.wSecond<60 && dt.wMilliseconds>=0 && dt.wMilliseconds<1000);
}

inline bool IsLeapYear( WORD p_year )
{
  return ((p_year % 400 == 0)||(p_year % 4 == 0))&&(p_year % 100 != 0);
}

inline int YearsBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  return p_then.wYear - p_now.wYear;
}

inline int MonthsBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  if (! IsValidateDate(p_now) || ! IsValidateDate(p_then))
    return 0;
  return YearsBetween(p_now,p_then)*12+(p_then.wMonth - p_now.wMonth);
}

int DaysBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  if (!IsValidateDate(p_now) || !IsValidateDate(p_then))
    return 0;
  SYSTEMTIME dtNow,dtThen;
  int x;
  if (p_now.wYear < p_then.wYear
    || (p_now.wYear == p_then.wYear && p_now.wMonth < p_then.wMonth)
    || (p_now.wYear == p_then.wYear && p_now.wMonth == p_then.wMonth && p_now.wDay < p_then.wDay)
    )
  {
    dtNow = p_now;
    dtThen = p_then;
    x = 1;
  }
  else
  {
    dtNow = p_then;
    dtThen = p_now;
    x = -1;
  }
  int ret = 0;
  while(true)
  {
    if (dtNow.wYear == dtThen.wYear && dtNow.wMonth == dtThen.wMonth && dtNow.wDay == dtThen.wDay)
      break;
    ++ret;
    ++(dtNow.wDay);

    if (dtNow.wMonth == 2)
    {
      if (dtNow.wDay>28 && !IsLeapYear(dtNow.wYear))
      {
        dtNow.wMonth = 3;
        dtNow.wDay = 1;
      }
      else if (dtNow.wDay>29)
      {
        dtNow.wMonth = 3;
        dtNow.wDay = 1;
      }
    }
    else if (dtNow.wDay > 30 && (dtNow.wMonth == 4 || dtNow.wMonth == 6 || dtNow.wMonth == 9 || dtNow.wMonth == 11))
    {
      ++(dtNow.wMonth);
      dtNow.wDay = 1;
    }
    else if (dtNow.wDay > 31 && (dtNow.wMonth == 1 || dtNow.wMonth == 3 || dtNow.wMonth == 5 || dtNow.wMonth == 7 || dtNow.wMonth == 8 || dtNow.wMonth == 10 || dtNow.wMonth == 12))
    {
      ++(dtNow.wMonth);
      dtNow.wDay = 1;
      if (dtNow.wMonth > 12)
      {
        ++(dtNow.wYear);
        dtNow.wMonth = 1;
      }
    }
  }

  return ret*x;
}

int HoursBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  if (! IsValidateDateTime(p_now) || ! IsValidateDateTime(p_then))
    return 0;
  return DaysBetween(p_now,p_then)*24+(p_then.wHour-p_now.wHour);
}

int MinutesBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  if (! IsValidateDateTime(p_now) || ! IsValidateDateTime(p_then))
    return 0;
  return HoursBetween(p_now,p_then)*60+(p_then.wMinute-p_now.wMinute);
}

long long SecondsBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
	return MinutesBetween(p_now,p_then)*60+(p_then.wSecond-p_now.wSecond);
}

long long MillisecondsBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then)
{
	return SecondsBetween(p_now,p_then)*1000 + (p_then.wMilliseconds - p_now.wMilliseconds);
}

inline void IncYear( SYSTEMTIME &dt,int years )
{
  dt.wYear += years;
}

//just internal use for IncMonth
void IncMonth_( SYSTEMTIME &dt,int months )
{
  int y = months / 12;
  int m = months % 12;
  IncYear(dt,y);
  while (m>0)
  {
    --m;
    ++(dt.wMonth);
    if (dt.wMonth > 12)
    {
      ++(dt.wYear);
      dt.wMonth = 1;
    }
  }
  if (dt.wMonth == 2)
  {
    if (dt.wDay > 29 && IsLeapYear(dt.wYear))
      dt.wDay = 29;
    if (dt.wDay > 28 && !IsLeapYear(dt.wYear))
      dt.wDay = 28;
  }
  else if (dt.wMonth == 4 || dt.wMonth == 6 || dt.wMonth == 9 || dt.wMonth == 11)
  {
    if (dt.wDay > 30)
      dt.wDay = 30;
  }

}

//just internal use for IncMonth
void DecMonth( SYSTEMTIME &dt,int months )
{
  int ms = months * -1;
  int y = ms / 12;
  int m = ms % 12;
  IncYear(dt,y * -1);
  while (m>0)
  {
    --m;
    --(dt.wMonth);
    if (dt.wMonth < 1)
    {
      --(dt.wYear);
      dt.wMonth = 12;
    }
  }
  if (dt.wMonth == 2)
  {
    if (dt.wDay > 29 && IsLeapYear(dt.wYear))
      dt.wDay = 29;
    if (dt.wDay > 28 && !IsLeapYear(dt.wYear))
      dt.wDay = 28;
  }
  else if (dt.wMonth == 4 || dt.wMonth == 6 || dt.wMonth == 9 || dt.wMonth == 11)
  {
    if (dt.wDay > 30)
      dt.wDay = 30;
  }

}

void IncMonth( SYSTEMTIME &dt,int months )
{
  if (! IsValidateDate(dt))
    return;
  if (months>=0)
    IncMonth_(dt,months);
  else
    DecMonth(dt,months);
}

//just internal use for IncDay
void IncDay_( SYSTEMTIME &dt,int days )
{
  int d = days;
  while (d>0)
  {
    --d;
    ++(dt.wDay);
    if (dt.wMonth == 2)
    {
      if (dt.wDay > 29 && IsLeapYear(dt.wYear))
      {
        dt.wDay = 1;
        dt.wMonth = 3;
      }
      if (dt.wDay > 28 && !IsLeapYear(dt.wYear))
      {
        dt.wDay = 1;
        dt.wMonth = 3;
      }
    }
    else if (dt.wMonth == 4 || dt.wMonth == 6 || dt.wMonth == 9 || dt.wMonth == 11)
    {
      if (dt.wDay > 30)
      {
        dt.wDay = 1;
        ++(dt.wMonth);
      }
    }
    else
    {
      if (dt.wDay > 31)
      {
        dt.wDay = 1;
        ++(dt.wMonth);
      }
    }
    if (dt.wMonth > 12)
    {
      dt.wMonth = 1;
      ++(dt.wYear);
    }
  }
}

//just internal use for IncDay
void DecDay( SYSTEMTIME &dt,int days )
{
  int d = days;
  while (d<0)
  {
    ++d;
    --(dt.wDay);
    if (dt.wDay < 1)
    {
      --(dt.wMonth);
      if (dt.wMonth < 1)
      {
        dt.wMonth = 12;
        --(dt.wYear);
      }
      if (dt.wMonth == 2)
      {
        if (IsLeapYear(dt.wYear))
          dt.wDay = 29;
        else
          dt.wDay = 28;
      }
      else if (dt.wMonth == 4 || dt.wMonth == 6 || dt.wMonth == 9 || dt.wMonth == 11)
      {
        dt.wDay = 30;
      }
      else
      {
        dt.wDay = 31;
      }
    }
  }
}

void IncDay( SYSTEMTIME &dt,int days )
{
  if (! IsValidateDate(dt))
    return;
  if (days >= 0)
    IncDay_(dt,days);
  else
    DecDay(dt,days);
}

void IncHour( SYSTEMTIME &dt,int hours )
{
  if (! IsValidateDateTime(dt))
    return;
  int d = hours / 24;
  int h = hours % 24;
  IncDay(dt,d);
  if (hours >= 0)
  {
    if (dt.wHour+h > 23)
    {
      IncDay(dt,1);
      dt.wHour = (dt.wHour + h - 24);
    }
    else
      dt.wHour += h;
  }
  else
  {
    int i = dt.wHour;
    i += h;
    if (i < 0)
    {
      IncDay(dt,-1);
      dt.wHour = 24 + h;
    }
    else
      dt.wHour = i;
  }
}

void IncMinute( SYSTEMTIME &dt,int minutes )
{
  if (! IsValidateDateTime(dt))
    return;
  int h = minutes / 60;
  int m = minutes % 60;
  IncHour(dt,h);
  if (minutes >= 0)
  {
    if (dt.wMinute+m > 59)
    {
      IncHour(dt,1);
      dt.wMinute = (dt.wMinute + m - 60);
    }
    else
      dt.wMinute = dt.wMinute + m;
  }
  else
  {
    int i = dt.wMinute;
    i += m;
    if (i < 0)
    {
      IncHour(dt,-1);
      dt.wMinute = 60 + m;
    }
    else
      dt.wMinute = i;
  }
}

void IncSecond( SYSTEMTIME &dt,int seconds )
{
  if (! IsValidateDateTime(dt))
    return;
  int m = seconds / 60;
  int s = seconds % 60;
  IncMinute(dt,m);
  if (seconds >= 0)
  {
    if (dt.wSecond+s > 59)
    {
      IncMinute(dt,1);
      dt.wSecond = (dt.wSecond + s - 60);
    }
    else
      dt.wSecond = dt.wSecond + s;
  }
  else
  {
    int i = dt.wSecond;
    i += s;
    if (i < 0)
    {
      IncHour(dt,-1);
      dt.wSecond = 60 + s;
    }
    else
      dt.wSecond = i;
  }
}

int DayOfWeek( SYSTEMTIME &dt )
{
  if (! IsValidateDate(dt))
   return -1;
  SYSTEMTIME st;
  GetLocalTime(&st);
  int ispan = DaysBetween(st,dt);
  int ispan1 = ispan % 7;
  if (ispan > 0)
  {
    dt.wDayOfWeek = st.wDayOfWeek + ispan1;
    if (dt.wDayOfWeek > 6)
      dt.wDayOfWeek -= 7;
  }
  else
  {
    int wd = st.wDayOfWeek;
    wd += ispan1;
    if (wd < 0)
      dt.wDayOfWeek = 7 + wd;
    else
      dt.wDayOfWeek = wd;
  }
  return dt.wDayOfWeek;
}

void IncWeek( SYSTEMTIME &dt,int weeks )
{
  if(! IsValidateDate(dt))
    return;
  IncDay(dt,weeks*7);
}

int WeeksBetween( SYSTEMTIME &p_now, SYSTEMTIME &p_then )
{
  return DaysBetween(p_now,p_then) / 7;
}

}
