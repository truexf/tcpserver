/*
 * sysutils.cpp
 *
 *  Created on: Jan 22, 2015
 *      Author: root
 */

#include "sysutils.h"
#include "AutoObject.hpp"
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include <ctype.h>
#include <string.h>
#include <algorithm>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include "tthread.h"
#include <execinfo.h>

using namespace std;

namespace fyslib
{
string GetBacktraceStr(int signo) {
    string ret(FormatString("signo %d,call stack:\n",signo));
    void *pTrace[256];
    char **ppszMsg = NULL;
    size_t uTraceSize = 0;
    do {
        if (0 == (uTraceSize = backtrace(pTrace, sizeof(pTrace) / sizeof(void *)))) {
            break;
        }
        if (NULL == (ppszMsg = backtrace_symbols(pTrace, uTraceSize))) {
            break;
        }
        for (size_t i = 0; i < uTraceSize; ++i) {
              ret += FormatString("%s\n", ppszMsg[i]);
        }
    } while (0);

    if (NULL != ppszMsg) {
        free(ppszMsg);
        ppszMsg = NULL;
    }
    return ret;
}
/*
 string 转换为 wstring
 */
std::wstring c2w(const char *pc)
{
	std::wstring val = L"";

	if (NULL == pc)
	{
		return val;
	}
	//size_t size_of_ch = strlen(pc)*sizeof(char);
	//size_t size_of_wc = get_wchar_size(pc);
	size_t size_of_wc;
	size_t destlen = mbstowcs(0, pc, 0);
	if (destlen == (size_t) (-1))
	{
		return val;
	}
	size_of_wc = destlen + 1;
	wchar_t * pw = new wchar_t[size_of_wc];
	mbstowcs(pw, pc, size_of_wc);
	val = pw;
	delete pw;
	return val;
}
/*
 wstring 转换为 string
 */
std::string w2c(const wchar_t * pw)
{
	std::string val = "";
	if (!pw)
	{
		return val;
	}
	size_t size = wcslen(pw) * sizeof(wchar_t);
	char *pc = NULL;
	if (!(pc = (char*) malloc(size)))
	{
		return val;
	}
	size_t destlen = wcstombs(pc, pw, size);
	/*转换不为空时，返回值为-1。如果为空，返回值0*/
	if (destlen == (size_t) (0))
	{
		return val;
	}
	val = pc;
	delete pc;
	return val;
}

wstring string2wstring(const string &s)
{
	return c2w(s.c_str());
}
string wstring2string(const wstring &s)
{
	return w2c(s.c_str());
}

string trim(const string &str)
{
	if (str == "")
		return str;
	int i = -1, j = -1;
	size_t k;
	for (k = 0; k < str.length(); ++k)
	{
		if (unsigned(str[k]) > unsigned(' '))
		{
			i = k;
			break;
		}
	}
	for (k = str.length() - 1; k >= 0; --k)
	{
		if (unsigned(str[k]) > unsigned(' '))
		{
			j = k;
			break;
		}
	}
	i = (i == -1 ? 0 : i);
	j = (j == -1 ? str.length() - 1 : j);
	return str.substr(i, j - i + 1);
}

std::wstring trimW(const wstring &str)
{
	if (str.empty())
		return str;
	int i = -1, j = -1;
	size_t k;
	for (k = 0; k < str.length(); ++k)
	{
		if (unsigned(str[k]) > unsigned(L' '))
		{
			i = k;
			break;
		}
	}
	for (k = str.length() - 1; k >= 0; --k)
	{
		if (unsigned(str[k]) > unsigned(L' '))
		{
			j = k;
			break;
		}
	}
	i = (i == -1 ? 0 : i);
	j = (j == -1 ? str.length() - 1 : j);
	return str.substr(i, j - i + 1);
}

bool FileExists(const char *fn)
{
	return 0 == access(fn, R_OK);
}

bool IsDir(const char *fn)
{
	struct stat fs;
	if (-1 == stat(fn, &fs))
		return false;
	return S_ISDIR(fs.st_mode);
}

long GetFileSize(const char *fn)
{
	struct stat fs;
	if (-1 == stat(fn, &fs))
		return -1;
	return (long) fs.st_size;
}

string ExtractFilePath(const string &filestr, bool IncludeBackslash)
{
	if (filestr == "")
		return "";
	for (int i = filestr.length() - 1; i >= 0; --i)
	{
		if (filestr[i] == '/')
		{
			if (IncludeBackslash)
				return filestr.substr(0, i + 1);
			else
				return filestr.substr(0, i);
		}
	}
	return "";
}

string ExtractFileName(const string &filestr)
{
	if (filestr == "")
		return "";
	for (int i = filestr.length() - 1; i >= 0; --i)
	{
		if (filestr[i] == '/')
		{
			return filestr.substr(i + 1);
		}
	}
	return "";
}

string IncludeTrailingPathDelimiter(const string &path)
{
	string s = trim(path);
	if (s.empty())
		return s;
	if (s[s.length() - 1] != '/')
		return s + "/";
	else
		return s;
}

string ExcludeTrailingPathDelimiter(const string &path)
{
	string s = trim(path);
	if (s.empty())
		return s;
	if (s[s.length() - 1] == '/')
		return s.substr(0, s.length() - 1);
	else
		return s;
}

bool ForceDirectory(const string &fn)
{
	bool ret = true;
	if (fn.empty())
		return false;
	string sdir = ExcludeTrailingPathDelimiter(fn);
	if (sdir.length() < 3 || DirectoryExists(sdir.c_str())
			|| (ExtractFilePath(sdir, true) == sdir))
		return ret;
	return ForceDirectory(ExtractFilePath(sdir, true))
			&& 0
					== mkdir(sdir.c_str(),
							S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP
									| S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
}

string UpperCase(const string &str)
{
	string ret(str);
	if (ret.empty())
		return ret;
	for (size_t i = 0; i < ret.length(); ++i)
	{
		ret[i] = toupper((int) ret[i]);
	}
	return ret;
}

string LowerCase(const string &str)
{
	string ret(str);
	if (ret.empty())
		return ret;
	for (size_t i = 0; i < ret.length(); ++i)
	{
		ret[i] = tolower((int) ret[i]);
	}
	return ret;
}

wstring UpperCaseW(const wstring &str)
{
	wstring ret(str);
	if (ret.empty())
		return ret;
	for (size_t i = 0; i < ret.length(); ++i)
	{
		ret[i] = toupper((int) ret[i]);
	}
	return ret;
}

wstring LowerCaseW(const wstring &str)
{
	wstring ret(str);
	if (ret.empty())
		return ret;
	for (size_t i = 0; i < ret.length(); ++i)
	{
		ret[i] = tolower((int) ret[i]);
	}
	return ret;
}

void SplitString(const string &AString, const string &ASplitStr,
		vector<string> &AStrings)
{
	if (ASplitStr.empty())
	{
		AStrings.clear();
		vector<string> v1, v2, v3;
		SplitString(AString, "\r\n", v1);
		for (vector<string>::iterator it = v1.begin(); it != v1.end(); ++it)
		{
			v2.clear();
			SplitString(*it, "\r", v2);
			for(size_t i = 0;i<v2.size();++i)
				v3.push_back(v2[i]);
		}
		for (vector<string>::iterator it = v3.begin(); it != v3.end(); ++it)
		{
			v2.clear();
			SplitString(*it, "\n", v2);
			for(size_t i = 0;i<v2.size();++i)
				AStrings.push_back(v2[i]);
		}
		return;
	}
	else
	{
		const char *sdata = AString.c_str();
		const char *ssplit = ASplitStr.c_str();
		size_t i = 0, j = 0, iSizeA = AString.length(), iSizeB =
				ASplitStr.length();
		AStrings.clear();
		while (i < iSizeA)
		{
			if (i + iSizeB > iSizeA)
			{
				AStrings.push_back(AString.substr(j));
				j = i;
				break;
			}
			if (memcmp(&(sdata[i]), ssplit, iSizeB) == 0)
			{
				//if (j < i)
				AStrings.push_back(AString.substr(j, i - j));
				j = i + iSizeB;
				i += iSizeB;
			}
			else
				++i;
		}
		if (j < i)
			AStrings.push_back(AString.substr(j));
		return;
	}
}

void SplitString(const string &AString, const string &ASplitStr, string &ALeft,
		string &ARight)
{
	vector<string> vs;
	SplitString(AString, ASplitStr, vs);
	if (vs.empty())
		return;
	ALeft = vs[0];
	ARight = "";
	if (vs.size() > 1)
	{
		for (size_t i = 1; i != vs.size(); ++i)
		{
			if (ARight.empty())
				ARight = vs[i];
			else
				ARight = ARight + ASplitStr + vs[i];
		}
	}
	else
		ARight = "";
	return;
}

void SplitStringW(const wstring &str, const wstring &splitstr,
		vector<wstring> &dest)
{
	if (splitstr.empty())
	{
		dest.clear();
		vector<wstring> v1, v2, v3;
		SplitStringW(str, L"\r\n", v1);
		for (vector<wstring>::iterator it = v1.begin(); it != v1.end(); ++it)
		{
			v2.clear();
			SplitStringW(*it, L"\r", v2);
			v3.insert(v3.end(), v2.begin(), v2.end());
		}
		for (vector<wstring>::iterator it = v3.begin(); it != v3.end(); ++it)
		{
			v2.clear();
			SplitStringW(*it, L"\r", v2);
			dest.insert(dest.end(), v2.begin(), v2.end());
		}
	}
	else
	{
		wstring::size_type pos = wstring::npos;
		wstring::size_type offset = 0;
		dest.clear();
		while (1)
		{
			pos = str.find(splitstr.c_str(), offset);
			if (wstring::npos == pos)
				break;
			dest.push_back(str.substr(offset, pos - offset));
			offset = splitstr.length() + pos;

		}
		if (offset != str.length())
			dest.push_back(str.substr(offset));
	}
}

void SplitStringW(const wstring &AString, const wstring &ASplitStr,
		wstring &ALeft, wstring &ARight)
{
	vector<wstring> vs;
	SplitStringW(AString, ASplitStr, vs);
	ALeft = vs[0];
	if (vs.size() > 1)
	{
		for (size_t i = 1; i != vs.size(); ++i)
		{
			if (ARight.empty())
				ARight = vs[i];
			else
				ARight = ARight + ASplitStr + vs[i];
		}
	}
	else
		ARight = L"";
	return;
}

string StringList2String(const vector<string> &AStringList,
		const string &AConnectStr)
{
	string ret;
	for (vector<string>::const_iterator it = AStringList.begin();
			it != AStringList.end(); ++it)
	{
		if (ret.empty())
			ret = *it;
		else
			ret = ret + AConnectStr + *it;
	}
	return ret;
}

wstring StringList2StringW(const vector<wstring> &AStringList,
		const wstring &AConnectStr)
{
	wstring ret;
	for (vector<wstring>::const_iterator it = AStringList.begin();
			it != AStringList.end(); ++it)
	{
		if (ret.empty())
			ret = *it;
		else
			ret = ret + AConnectStr + *it;
	}
	return ret;
}

string& ReplaceString(string &s, const string &OldPattern,
		const string &NewPattern, bool bReplaceAll)
{
	if (OldPattern == NewPattern)
		return s;
	string::size_type i;
	while (true)
	{
		i = s.find(OldPattern);
		if (i == string::npos)
			break;
		else
		{
			s = s.replace(i, OldPattern.length(), NewPattern);
			if (!bReplaceAll)
				break;
		}
	}
	return s;
}

wstring& ReplaceStringW(wstring &s, const wstring &OldPattern,
		const wstring &NewPattern, bool bReplaceAll)
{
	if (OldPattern == NewPattern)
		return s;
	wstring::size_type i;
	while (true)
	{
		i = s.find(OldPattern);
		if (i == wstring::npos)
			break;
		else
		{
			s = s.replace(i, OldPattern.length(), NewPattern);
			if (!bReplaceAll)
				break;
		}
	}
	return s;
}

string& ReplaceStringI(string &s, const string &OldPattern,
		const string &NewPattern, bool bReplaceAll)
{
	if (0 == strcasecmp(OldPattern.c_str(), NewPattern.c_str()))
		return s;
	string::iterator it, itend;
	do
	{
		it = search(s.begin(), s.end(), OldPattern.begin(), OldPattern.end(),
				CharCmpI());
		if (it != s.end())
		{
			itend = it + OldPattern.length();
			s = s.replace(it, itend, NewPattern);
			if (!bReplaceAll)
				break;
		}
		else
			break;
	} while (true);
	return s;
}

wstring& ReplaceStringIW(wstring &s, const wstring &OldPattern,
		const wstring &NewPattern, bool bReplaceAll)
{
	if (0 == wcscasecmp(OldPattern.c_str(), NewPattern.c_str()))
		return s;
	wstring::iterator it, itend;
	do
	{
		it = search(s.begin(), s.end(), OldPattern.begin(), OldPattern.end(),
				CharCmpIW());
		if (it != s.end())
		{
			itend = it + OldPattern.length();
			s = s.replace(it, itend, NewPattern);
			if (!bReplaceAll)
				break;
		}
		else
			break;
	} while (true);
	return s;
}

string Int2Str(long i)
{
	char ret[20] =
	{ 0 };
	sprintf(ret, "%ld", i);
	return ret;
}

void GetCommandLineList(vector<string> &lst)
{
	lst.clear();
	string fncmdline;
	fncmdline = fncmdline + "/proc/" + Int2Str(getpid()) + "/cmdline";
	if (FileExists(fncmdline.c_str()))
	{
		char scmds[1024] =
		{ 0 };
		int fd = open(fncmdline.c_str(), O_RDONLY);
		if (-1 == fd)
			return;
		if (-1 == read(fd, scmds, 1024))
		{
			close(fd);
			return;
		}
		close(fd);
		size_t j = 0;
		for (size_t i = 0; i < 1024 - 1; ++i)
		{
			if (0 == scmds[i] && 0 == scmds[i + 1])
			{
				j = i;
				break;
			}
		}
		if (0 == j)
			return;
		SplitString(string(scmds, j), string("\0", 1), lst);
	}
	char dir[1024] = {0};
	int n = readlink("/proc/self/exe", dir, 1024);
	if (n > 0 && lst.size() > 0)
	{
		lst[0] = string(dir);
	}
}

string ParamStr(size_t i)
{
	vector<string> v;
	GetCommandLineList(v);
	if (v.size() > i)
		return v[i];
	return "";
}
string LoadStringFromFile(const string &file)
{
    void *buf = NULL;
    size_t buf_len = 0;
    if (LoadBufferFromFile(file,&buf,buf_len)) {
        string ret((char*)buf,buf_len);
        free(buf);
        return ret;
    } else {
        return "";
    }
}
bool LoadBufferFromFile(const string &file,/*outer*/void **buf,/*outer*/
		size_t &bufsize)
{
	if (!FileExists(file.c_str()))
		return false;
	if (IsDir(file.c_str()))
		return false;
	long fsize = GetFileSize(file.c_str());
	if (-1 == fsize)
		return false;
	int fd = open(file.c_str(), O_RDONLY);
	if (-1 == fd)
		return false;
	*buf = malloc(fsize);
	bufsize = 0;
	void *p = *buf;
	long iretain = fsize;
	long irded = 0;
	while (iretain > 0)
	{
		irded = read(fd, p, iretain);
		if (-1 == irded)
		{
			free(*buf);
			*buf = NULL;
			close(fd);
			return false;
		}
		iretain -= irded;
		IncPtr(&p, irded);
	}
	close(fd);
	bufsize = fsize;
	return true;
}

bool SaveBufferToFile(const string &file, void *buf, size_t bufsize)
{
	if (file.empty())
		return false;
	if (NULL == buf || 0 == bufsize)
		return false;
	int fd = open(file.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (-1 == fd)
		return false;
	long iretain = bufsize;
	long iw = 0;
	void *p = buf;
	while (iretain > 0)
	{
		iw = write(fd, p, iretain);
		if (-1 == iw)
		{
			close(fd);
			return false;
		}
		iretain -= iw;
		IncPtr(&p, iw);
	}
	close(fd);
	return true;
}

bool AppendBuf2File(const string file, const void *buf, size_t bufsize)
{
	if (file.empty())
		return false;
	if (NULL == buf || 0 == bufsize)
		return false;
	int fd = open(file.c_str(), O_WRONLY | O_APPEND);
	if (-1 == fd)
		return false;
	long iretain = bufsize;
	long iw = 0;
	void *p = (void*) buf;
	while (iretain > 0)
	{
		iw = write(fd, p, iretain);
		if (-1 == iw)
		{
			close(fd);
			return false;
		}
		iretain -= iw;
		IncPtr(&p, iw);
	}
	close(fd);
	return true;
}

int CompareVersion(const string &ver1, const string &ver2)
{
	if (ver1.empty() && ver2.empty())
		return 0;
	if (ver1.empty() && !ver2.empty())
		return -1;
	if (!ver1.empty() && ver2.empty())
		return 1;

	vector<string> v1, v2;
	SplitString(ver1, ".", v1);
	SplitString(ver2, ".", v2);
	for (size_t i = 0; i < v1.size(); ++i)
	{
		if (i >= v2.size())
			return 1;
		if (atoi(v1[i].c_str()) < atoi(v2[i].c_str()))
			return -1;
		if (atoi(v1[i].c_str()) > atoi(v2[i].c_str()))
			return 1;
	}
	if (v1.size() < v2.size())
		return -1;
	return 0;
}

//测试字符串匹配，只支持通配符*,表示0个或多个字符，忽略大小写
bool SimpleMatch(const string &afmt, const string& astr)
{
	string sfmt = LowerCase(afmt);
	string sstr = LowerCase(astr);
	if (sfmt == sstr)
		return true;
	const char* fmt = sfmt.c_str();
	const char* str = sstr.c_str();
	int fmt_len = sfmt.length(), str_len = sstr.length();
	int i = 0, j = 0;
	int ib, ie;
	bool bmatched;
	while (i < fmt_len && j < str_len)
	{
		if (fmt[i] == '*') //遇到*，取*后面的那段进行匹配
		{
			while (i < fmt_len && fmt[i] == '*')
				++i;
			ib = i;
			while (i < fmt_len && fmt[i] != '*')
				++i;
			ie = i;
			if (fmt[ib] == fmt[ie])
				return true;
			i = ib;
			if (j + ie - ib >= str_len)
				return false;

			bmatched = false;
			while (str_len - j >= ie - ib)
			{
				if (memcmp(&fmt[ib], &str[j], ie - ib) == 0)
				{
					j += ie - ib;
					i = ie;
					bmatched = true;
					break;
				}
				else
					++j;
			}
			if (!bmatched)
				return false;
		}
		else
		{
			if (fmt[i] != str[j])
				return false;
			else
			{
				++i;
				++j;
			}
		}
	}

	if (i < fmt_len && j >= str_len) //这种情况下判断fmt的末尾是否为*，若是，则匹配成功
	{
		bool b = false;
		for (int k = i; k < fmt_len; ++k)
		{
			if (fmt[k] != '*')
			{
				b = true;
				break;
			}
		}
		if (!b)
			return true;
	}
	if (i < fmt_len || j < str_len)
		return false;
	return true;
}

string Fmt(const char* str, ...) {
    string ret = str;
    if (str != NULL)
    {
        char vret[8192] = { 0 };
        va_list vl;
        va_start(vl, str);
        vsnprintf(vret, 8192, str, vl);
        va_end(vl);
        return string(vret, strlen(vret));
    }
    return "";
}
string FormatString(const char * str, ...)
{
	string ret = str;
	if (str != NULL)
	{
		char vret[1024] =
		{ 0 };
		va_list vl;
		va_start(vl, str);
		vsnprintf(vret, 1024, str, vl);
		va_end(vl);
		return string(vret, strlen(vret));
	}
	return "";
}

wstring FormatStringW(const wchar_t *str, ...)
{
	if (str != NULL)
	{
		wchar_t vret[1024] =
		{ 0 };
		va_list vl;
		va_start(vl, str);
		vswprintf(vret, 1024, str, vl);
		va_end(vl);
		return wstring(vret, wcslen(vret));
	}
	return L"";
}

string FormatStringEx(size_t buf_size, const char* str, ...)
{
	if (str != NULL)
	{
		char *vret = (char*) malloc(buf_size);
		memset(vret, 0, buf_size);
		va_list vl;
		va_start(vl, str);
		vsnprintf(vret, buf_size, str, vl);
		va_end(vl);
		string ret(vret, strlen(vret));
		::free(vret);
		return ret;
	}
	return "";
}

wstring FormatStringWEx(size_t buf_size, const wchar_t* str, ...)
{
	if (str != NULL)
	{
		wchar_t *vret = (wchar_t*) malloc(buf_size * sizeof(wchar_t));
		memset(vret, 0, buf_size * sizeof(wchar_t));
		va_list vl;
		va_start(vl, str);
		vswprintf(vret, buf_size, str, vl);
		va_end(vl);
		wstring ret(vret, wcslen(vret));
		::free(vret);
		return ret;
	}
	return L"";
}

string FillString(string s,size_t width,char fillChar,bool fillLeft)
{
	if (width <= s.length())
		return s;
	vector<char> v(width,fillChar);
	if (fillLeft)
	{
		size_t j = 0;
		for(size_t i=width-s.length();i<width;++i)
		{
			v[i] = s[j];
			++j;
		}
	}
	else
	{
		for(size_t i = 0;i<s.length();++i)
		{
			v[i] = s[i];
		}
	}
	return string((const char*)v.data(),width);
}

string CreateGUID()
{
	 uuid_t uuid;
	 char str[36];
	 ::uuid_generate(uuid);
	 ::uuid_unparse(uuid, str);
	 return string(str,36);
}

string BufToHex(unsigned char *buf, unsigned long bufsize)
{
	if (NULL == buf || bufsize == 0)
		return "";
	char *ret = (char*) malloc(bufsize * 2);
	if (!ret)
		return "";
	char m[17] = "0123456789ABCDEF";
	unsigned char L, R;
	int j = 0;
	for (unsigned long i = 0; i != bufsize; ++i)
	{
		L = (unsigned char) buf[i];
		L >>= 4;
		R = (unsigned char) buf[i];
		R <<= 4;
		R >>= 4;
		ret[j] = m[L];
		ret[j + 1] = m[R];
		j += 2;
	}
	string sret;
	sret.assign(ret, bufsize * 2);
	free(ret);
	return sret;
}

void HexToBuf(const string &strhex, /*out*/unsigned char **buf, /*out*/
		unsigned long &bufsize)
{
	if (strhex.length() < 2)
	{
		*buf = NULL;
		bufsize = 0;
		return;
	}
	const unsigned char tbl[256] =
	{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			255, 255, 255, 255, 255, 255, 255, 10, 11, 12, 13, 14, 15, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 10, 11, 12,
			13, 14, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
	*buf = (unsigned char*) malloc(strhex.length() / 2);
	unsigned long j = 0;
	for (unsigned long i = 0; i < strhex.length() - 1; i += 2)
	{
		unsigned char L, R;
		L = tbl[(unsigned long) strhex[i]];
		L <<= 4;
		R = tbl[(unsigned long) strhex[i + 1]];
		(*buf)[j] = (L | R);
		++j;
	}
	bufsize = strhex.length() / 2;
}

string ReadBufferIni(const string &AIniData, const string &ASection,
		const string &AIdent, const string &ADefaultValue)
{
	vector<string> sl;
	SplitString(AIniData, "", sl);
	for (size_t i = 0; i < sl.size(); ++i)
	{
		sl[i] = trim(sl[i]);
	}

	return ReadBufferIni(sl, ASection, AIdent, ADefaultValue);
}

string ReadBufferIni(const vector<string> &AIniData, const string &ASection,
		const string &AIdent, const string &ADefaultValue)
{
	const vector<string> sl = AIniData;
	string sLeft, sRight;
	string ret = ADefaultValue;

	for (vector<string>::size_type i = 0; i != sl.size(); ++i)
	{
		if (SameText("[" + ASection + "]", sl[i]))
		{
			for (vector<string>::size_type j = i + 1; j != sl.size(); ++j)
			{
				if (sl[j][1] == '[')
					return ret;
				else
				{
					SplitString(sl[j], "=", sLeft, sRight);
					if (SameText(sLeft, AIdent))
						return sRight;
				}
			}
			return ret;
		}
	}
	return ret;
}

void _rc4_init(unsigned char *s, unsigned char *key, unsigned long Len) //初始化函数
{
	int i =0, j = 0;
	char k[256] = {0};
	unsigned char tmp = 0;
	for (i=0;i<256;i++) {
		s[i] = i;
		k[i] = key[i%Len];
	}
	for (i=0; i<256; i++) {
		j=(j+s[i]+k[i])%256;
		tmp = s[i];
		s[i] = s[j]; //交换s[i]和s[j]
		s[j] = tmp;
	}
}

void _rc4_crypt(unsigned char *s, unsigned char *Data, unsigned long Len) //加解密
{
	int i = 0, j = 0, t = 0;
	unsigned long k = 0;
	unsigned char tmp;
	for(k=0;k<Len;k++) {
		i=(i+1)%256;
		j=(j+s[i])%256;
		tmp = s[i];
		s[i] = s[j]; //交换s[x]和s[y]
		s[j] = tmp;
		t=(s[i]+s[j])%256;
		Data[k] ^= s[t];
	}
}

void rc4_crypt(unsigned char *key, unsigned long keyLen,unsigned char *data, unsigned long dataLen)
{
	unsigned char s[256] = {0}; //S-box
	_rc4_init(s,key,keyLen);
	_rc4_crypt(s,data,dataLen);
}

string rc4_string(string key,string data)
{
	unsigned char *ret = (unsigned char*)malloc(data.length());
	memcpy(ret,data.c_str(),data.length());
	rc4_crypt((unsigned char*)key.c_str(),key.length(),ret,data.length());
	string sret((char*)ret,data.length());
	free(ret);
	return sret;
}

const char base_trad[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
const char base_url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";
//const char base_url[] = "oMq_rc1fuEbtXgBOkKlIxyzNFYd2Js0D-Tv8495A6iheCpWS3QGUamwVjRH7nLPZ=";
char *base64_encode(const char* data, int data_len,bool urlencode) {
	char *base;
	if (!urlencode)
		base = (char*)base_trad;
	else
		base = (char*)base_url;
	int prepare = 0;
	int ret_len;
	int temp = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	char changed[4];
	int i = 0;
	ret_len = data_len / 3;
	temp = data_len % 3;
	if (temp > 0) {
		ret_len += 1;
	}
	ret_len = ret_len * 4 + 1;
	ret = (char *) malloc(ret_len);
	if (ret == NULL) {
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < data_len) {
		temp = 0;
		prepare = 0;
		memset(changed, '\0', 4);
		while (temp < 3) {
//printf("tmp = %d\n", tmp);
			if (tmp >= data_len) {
				break;
			}
			prepare = ((prepare << 8) | (data[tmp] & 0xFF));
			tmp++;
			temp++;
		}
		prepare = (prepare << ((3 - temp) * 8));
//printf("before for : temp = %d, prepare = %d\n", temp, prepare);
		for (i = 0; i < 4; i++) {
			if (temp < i) {
				changed[i] = 0x40;
			} else {
				changed[i] = (prepare >> ((3 - i) * 6)) & 0x3F;
			}
			*f = base[changed[i]];
//printf("%.2X", changed[i]);
			f++;
		}
	}
	*f = '\0';
	return ret;
}
/* */
static char find_pos(char ch,bool urlencode) {
	char *base;
	if (!urlencode)
		base = (char*)base_trad;
	else
		base = (char*)base_url;
	char *ptr = (char*) strrchr(base, ch); //the last position (the only) in base[]
	return (ptr - base);
}
/* */
char *base64_decode(const char *data, int data_len,bool urlencode) {
	char *base;
	if (!urlencode)
		base = (char*)base_trad;
	else
		base = (char*)base_url;
	int ret_len = (data_len / 4) * 3;
	int equal_count = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	int temp = 0;
	char need[3];
	int prepare = 0;
	int i = 0;
	if (*(data + data_len - 1) == '=') {
		equal_count += 1;
	}
	if (*(data + data_len - 2) == '=') {
		equal_count += 1;
	}
	if (*(data + data_len - 3) == '=') { //seems impossible
		equal_count += 1;
	}
	switch (equal_count) {
	case 0:
		ret_len += 4; //3 + 1 [1 for NULL]
		break;
	case 1:
		ret_len += 4; //Ceil((6*3)/8)+1
		break;
	case 2:
		ret_len += 3; //Ceil((6*2)/8)+1
		break;
	case 3:
		ret_len += 2; //Ceil((6*1)/8)+1
		break;
	}
	ret = (char *) malloc(ret_len);
	if (ret == NULL) {
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < (data_len - equal_count)) {
		temp = 0;
		prepare = 0;
		memset(need, 0, 4);
		while (temp < 4) {
			if (tmp >= (data_len - equal_count)) {
				break;
			}
			prepare = (prepare << 6) | (find_pos(data[tmp],urlencode));
			temp++;
			tmp++;
		}
		prepare = prepare << ((4 - temp) * 6);
		for (i = 0; i < 3; i++) {
			if (i == temp) {
				break;
			}
			*f = (char) ((prepare >> ((2 - i) * 8)) & 0xFF);
			f++;
		}
	}
	*f = '\0';
	return ret;
}

string base64_encode_string(string data,bool urlencode)
{
	char *ret = base64_encode(data.c_str(),data.length(),urlencode);
	string sret(ret);
	free(ret);
	return sret;
}
string base64_decode_string(string data,bool urlencode)
{
	char *ret = base64_decode(data.c_str(),data.length(),urlencode);
	string sret(ret);
	free(ret);
	return sret;
}

MemoryStream::MemoryStream() :
		m_buffer(NULL), m_size(0), m_pos(0), m_capacity(0), m_init_capacity(
				1024)
{

}

MemoryStream::~MemoryStream()
{
	Clear();
}

long MemoryStream::Read(MemoryStream &dest, long bytes)
{
	long new_bytes = min(m_size - m_pos, bytes);
	if (new_bytes <= 0)
		return 0;
	void *p = m_buffer;
	IncPtr(&p, m_pos);
	dest.Write(p, new_bytes);
	m_pos += new_bytes;
	return new_bytes;
}

long MemoryStream::Read(void *dest, long bytes)
{
	if (bytes <= 0)
		return 0;
	long ret = min(bytes, m_size - m_pos);
	if (ret <= 0)
		return 0;
	void *p = m_buffer;
	IncPtr(&p, m_pos);
	memcpy(dest, p, ret);
	m_pos += ret;
	return ret;
}

bool MemoryStream::Write(const MemoryStream &from, long bytes /*= -1*/)
{
	if (bytes < -1 || bytes == 0 || bytes > from.GetSize())
		return false;
	long lsize = 0;
	if (-1 == bytes)
		lsize = from.GetSize();
	else
		lsize = bytes;
	long add_size = lsize - (m_capacity - m_pos);
	while (add_size > 0)
	{
		if (!Expand())
			return false;
		add_size = lsize - (m_capacity - m_pos);
	}
	void *p = m_buffer;
	IncPtr(&p, m_pos);
	memcpy(p, from.GetBuffer(), lsize);
	m_pos += lsize;
	m_size = max(m_size, m_pos);
	return true;
}

bool MemoryStream::Write(const void *from, long bytes)
{
	if (bytes <= 0)
		return false;
	long add_size = bytes - (m_capacity - m_pos);
	while (add_size > 0)
	{
		if (!Expand())
			return false;
		add_size = bytes - (m_capacity - m_pos);
	}
	void *p = m_buffer;
	IncPtr(&p, m_pos);
	memcpy(p, from, bytes);
	m_pos += bytes;
	m_size = max(m_size, m_pos);
	return true;
}

void MemoryStream::Seek(SeekOrigin so, long offset)
{
	if (m_size <= 0)
		return;
	long os = 0;
	if (so == soBegin)
		os = offset;
	else if (so == soEnd)
		os = m_size - 1 + offset;
	else if (so == soCurrent)
		os = m_pos + offset;
	else
		return;
	if (os < 0)
		os = 0;
	else if (os >= m_size)
		os = m_size - 1;
	m_pos = os;
}

void MemoryStream::Clear()
{
	free(m_buffer);
	m_buffer = NULL;
	m_pos = 0;
	m_size = 0;
	m_capacity = 0;
}

bool MemoryStream::LoadFromFile(const string &file)
{
	void *buf = NULL;
	size_t bufsize = 0;
	if (LoadBufferFromFile(file, &buf, bufsize))
	{
		Clear();
		Write(buf, bufsize);
		free(buf);
		return true;
	}
	return false;
}

bool MemoryStream::SaveToFile(const string &file)
{
	return SaveBufferToFile(file, m_buffer, m_size);
}

bool MemoryStream::Expand(long new_capacity /*= -1*/)
{
	long newp = m_capacity * 2;
	if (0 == newp)
		newp = m_init_capacity;
	if (new_capacity != -1)
		newp = max(new_capacity, newp);
	if (newp <= m_capacity)
		return false;
	void *new_buf = NULL;
	if (m_buffer == NULL)
		new_buf = malloc(newp);
	else
		new_buf = realloc(m_buffer, newp);
	if (NULL == new_buf)
		return false;
	m_buffer = new_buf;
	m_capacity = newp;
	return true;
}

void MemoryStream::Empty()
{
	m_pos = 0;
	m_size = 0;
}

bool MemoryStream::Shrink(long new_size)
{
	if (new_size < 0 || new_size >= m_size)
		return false;
	m_size = new_size;
	if (m_pos >= m_size)
		m_pos = m_size - 1;
	return true;
}

ForwardBuffer::ForwardBuffer(){
	m_buffer = NULL;
	m_size = 0;
	m_pos = 0;
	m_capacity = 0;
	m_lock = CreateMutex(true);
}
ForwardBuffer::~ForwardBuffer(){
	if (m_buffer)
		free(m_buffer);
	DestroyMutex(m_lock);
}

POINTER ForwardBuffer::Read(void *dest, POINTER bytes){
	AutoMutex auto1(m_lock);
	if (m_buffer == NULL || m_size <= 0)
		return 0;
	long n = bytes;
	if (m_size < bytes){
		n = m_size;
	}
	memcpy(dest,AddPtr(m_buffer,m_pos),n);
	m_pos += n;
	m_size -= n;
	return n;
}
bool ForwardBuffer::Write(const void *from, POINTER bytes){
	AutoMutex auto1(m_lock);
	POINTER n = m_capacity - m_pos - m_size;
	if (n < bytes){
		Grow(bytes);
	}
	memcpy(AddPtr(m_buffer,m_pos + m_size),from,bytes);
	m_size += bytes;
	return true;
}
void ForwardBuffer::Grow(POINTER bytes){
	AutoMutex auto1(m_lock);
	POINTER n = m_capacity - m_pos + bytes;
	void *mem = malloc((size_t)n);
	if (m_buffer && m_size > 0){
		memcpy(mem,AddPtr(m_buffer,m_pos),m_size);
		free(m_buffer);
	}
	m_pos = 0;
	m_capacity = n;
	m_buffer = mem;
}
void ForwardBuffer::Reset(){
	m_size = 0;
	m_pos = 0;
}

}

