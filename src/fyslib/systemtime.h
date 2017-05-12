/*
 * systemtime.h
 *
 *  Created on: Jan 27, 2015
 *      Author: root
 */

#ifndef FYSLIB_SYSTEMTIME_H_
#define FYSLIB_SYSTEMTIME_H_

#include <string>
using namespace std;

namespace fyslib{

typedef unsigned short WORD;
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st);
//以yyyy-mm-dd hh:nn:ss:zzz的格式返回一个日期的字符串格式
string FormatDatetime(const SYSTEMTIME &st);
wstring FormatDatetimeW(const SYSTEMTIME &st);
string NowStr();

//以yyyy-mm-dd的格式返回一个日期的字符串格式
string FormatDate(const SYSTEMTIME &st);
wstring FormatDateW(const SYSTEMTIME &st);

//以hh:nn:ss的格式返回一个日期的字符串格式
string FormatTime(const SYSTEMTIME &st);
wstring FormatTimeW(const SYSTEMTIME &st);

//YYYY-MM-DD HH:NN:SS
bool Str2Datetime(const string &str, SYSTEMTIME &ret);
bool IsLeapYear(WORD p_year);
bool IsValidateDate(SYSTEMTIME &dt);
bool IsValidateDateTime(SYSTEMTIME &dt);
int YearsBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
int MonthsBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
int DaysBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
int HoursBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
int MinutesBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
long long SecondsBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
long long MillisecondsBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);
void IncYear(SYSTEMTIME &dt,int years);
void IncMonth(SYSTEMTIME &dt,int months);
void IncDay(SYSTEMTIME &dt,int days);
void IncHour(SYSTEMTIME &dt,int hours);
void IncMinute(SYSTEMTIME &dt,int minutes);
void IncSecond(SYSTEMTIME &dt,int seconds);
int DayOfWeek(SYSTEMTIME &dt); //0 sunday 1 monday 2 tuesday ...6 saturday
void IncWeek(SYSTEMTIME &dt,int weeks);
int WeeksBetween(SYSTEMTIME &p_now, SYSTEMTIME &p_then);

}
#endif /* FYSLIB_SYSTEMTIME_H_ */
