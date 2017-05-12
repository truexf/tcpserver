#ifndef SYSUTILS_H_
#define SYSUTILS_H_

#ifdef __x86_64__
#define POINTER long long
#else
#define POINTER	long
#endif

#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <pthread.h>
using namespace std;

namespace fyslib
{

string GetBacktraceStr(int signo);

inline void IncPtr(void **p, POINTER inc_bytes)
{
	*p = (void*) ((POINTER) *p + inc_bytes);
}

inline void *AddPtr(void *p, POINTER add_bytes)
{
	return (void*) ((POINTER) p + add_bytes);
}

struct CharCmpI
{
	bool operator()(char a, char b)
	{
		return toupper(a) == toupper(b);
	}
};

struct CharCmpIW
{
	bool operator()(wchar_t a, wchar_t b)
	{
		return toupper(a) == toupper(b);
	}
};

std::wstring c2w(const char *pc);
std::string w2c(const wchar_t * pw);
string Int2Str(long i);
inline wstring Int2StrW(long i)
{
	return c2w(Int2Str(i).c_str());
}
string trim(const string &str);
wstring trimW(const wstring &str);
string UpperCase(const string &str);
wstring UpperCaseW(const wstring &str);
string LowerCase(const string &str);
wstring LowerCaseW(const wstring &str);
inline bool SameText(const string &a, const string &b)
{
	return (strcasecmp(a.c_str(), b.c_str()) == 0);
}
inline bool SameTextW(const wstring &a, const wstring &b)
{
	return (wcscasecmp(a.c_str(), b.c_str()) == 0);
}
void SplitString(const string &AString, const string &ASplitStr,
		vector<string> &AStrings);
void SplitStringW(const wstring &str, const wstring &splitstr,
		vector<wstring> &dest);
void SplitString(const string &AString, const string &ASplitStr, string &ALeft,
		string &ARight);
void SplitStringW(const wstring &AString, const wstring &ASplitStr,
		wstring &ALeft, wstring &ARight);
string StringList2String(const vector<string> &AStringList,
		const string &AConnectStr);
wstring StringList2StringW(const vector<wstring> &AStringList,
		const wstring &AConnectStr);
string& ReplaceString(string &s, const string &OldPattern,
		const string &NewPattern, bool bReplaceAll);
wstring& ReplaceStringW(wstring &s, const wstring &OldPattern,
		const wstring &NewPattern, bool bReplaceAll);
string& ReplaceStringI(string &s, const string &OldPattern,
		const string &NewPattern, bool bReplaceAll = true);
wstring& ReplaceStringIW(wstring &s, const wstring &OldPattern,
		const wstring &NewPattern, bool bReplaceAll = true);
string FormatString(const char* str, ...);
string Fmt(const char* str, ...);
wstring FormatStringW(const wchar_t* str, ...);
string FormatStringEx(size_t buf_size, const char* str, ...);
wstring FormatStringWEx(size_t buf_size, const wchar_t* str, ...);
string FillString(string s,size_t width,char fillChar,bool fillLeft);

string CreateGUID();

bool FileExists(const char *fn);
inline bool FileExistsW(const wchar_t *fn)
{
	return FileExists(w2c(fn).c_str());
}
bool IsDir(const char *fn);
inline bool DirectoryExists(const char *fn)
{
	return FileExists(fn) && IsDir(fn);
}

inline bool DirectoryExistsW(const wchar_t *fn)
{
	return DirectoryExists(w2c(fn).c_str());
}

wstring string2wstring(const string &s);
string wstring2string(const wstring &s);
string ExtractFilePath(const string &filestr, bool IncludeBackslash);
inline wstring ExtractFilePathW(const wstring &filestr, bool IncludeBackslash)
{
	return c2w(ExtractFilePath(w2c(filestr.c_str()), IncludeBackslash).c_str());
}
string ExtractFileName(const string &filestr);
inline wstring ExtractFileNameW(const wstring &filestr)
{
	return c2w(ExtractFileName(w2c(filestr.c_str())).c_str());
}
string IncludeTrailingPathDelimiter(const string &path);
inline wstring IncludeTrailingPathDelimiterW(const wstring &path)
{
	return c2w(IncludeTrailingPathDelimiter(w2c(path.c_str())).c_str());
}
string ExcludeTrailingPathDelimiter(const string &path);
inline wstring ExcludeTrailingPathDelimiterW(const wstring &path)
{
	return c2w(ExcludeTrailingPathDelimiter(w2c(path.c_str())).c_str());
}
bool ForceDirectory(const string &fn);
inline bool ForceDirectoryW(const wstring &fn)
{
	return ForceDirectory(w2c(fn.c_str()));
}
long GetFileSize(const char *fn);
inline long GetFileSizeW(const wchar_t *fn)
{
	return GetFileSize(w2c(fn).c_str());
}
string LoadStringFromFile(const string &file);
bool LoadBufferFromFile(const string &file,/*outer*/void **buf,/*outer*/
		size_t &bufsize);
inline bool LoadBufferFromFileW(const wstring &file,/*outer*/void **buf,/*outer*/
		size_t &bufsize)
{
	return LoadBufferFromFile(w2c(file.c_str()), buf, bufsize);
}
bool SaveBufferToFile(const string &file, void *buf, size_t bufsize);
inline bool SaveBufferToFileW(const wstring &file, void *buf, size_t bufsize)
{
	return SaveBufferToFile(w2c(file.c_str()), buf, bufsize);
}
bool AppendBuf2File(const string file, const void *buf, size_t bufsize);
inline bool AppendBuf2FileW(const wstring file, const void *buf, size_t bufsize)
{
	return AppendBuf2File(w2c(file.c_str()), buf, bufsize);
}
//buffer转16进制
string BufToHex(unsigned char *buf, unsigned long bufsize);
//16进制转buffer
void HexToBuf(const string &strhex,
/*out*/unsigned char **buf,
/*out*/unsigned long &bufsize);

void GetCommandLineList(vector<string> &lst);
string ParamStr(size_t i);
inline wstring ParamStrW(size_t i)
{
	return c2w(ParamStr(i).c_str());
}

int CompareVersion(const string &ver1, const string &ver2);
bool SimpleMatch(const string &afmt, const string& astr);

string ReadBufferIni(const string &AIniData, const string &ASection,
		const string &AIdent, const string &ADefaultValue);
string ReadBufferIni(const vector<string> &AIniData, const string &ASection,
		const string &AIdent, const string &ADefaultValue);

void rc4_crypt(unsigned char *key, unsigned long keyLen,unsigned char *data, unsigned long dataLen);
string rc4_string(string key,string data);

char *base64_encode(const char* data, int data_len,bool urlencode);
char *base64_decode(const char* data, int data_len,bool urlencode);
string base64_encode_string(string data,bool urlencode);
string base64_decode_string(string data,bool urlencode);

class MemoryStream
{
public:
	enum SeekOrigin
	{
		soBegin, soEnd, soCurrent
	};
	MemoryStream();
	virtual ~MemoryStream();
	long Read(MemoryStream &dest, long bytes);
	long Read(void *dest, long bytes);
	string AsString()
	{
		return string((char*)GetBuffer(),GetSize());
	}
	bool Write(const MemoryStream &from, long bytes = -1);
	bool Write(const void *from, long bytes);
	bool WriteString(const string &s)
	{
		return Write(s.c_str(),s.length());
	}
	long GetSize() const
	{
		return m_size;
	}
	long GetPos() const
	{
		return m_pos;
	}
	long GetCapacity() const
	{
		return m_capacity;
	}
	void* GetBuffer() const
	{
		return m_buffer;
	}
	void Seek(SeekOrigin so, long offset);
	void Clear();
	bool LoadFromFile(const string &file);
	bool SaveToFile(const string &file);
	bool Expand(long new_capacity = -1); //-1 默认为当前容量的两倍
	void SetInitCapacity(long cc)
	{
		m_init_capacity = cc;
	}
	void Empty();
	bool Shrink(long new_size); //
private:
	MemoryStream(const MemoryStream&);
	MemoryStream& operator=(const MemoryStream&);
private:
	void *m_buffer;
	long m_size;
	long m_pos;
	long m_capacity;
	long m_init_capacity;
};

class ForwardBuffer
{
public:
	ForwardBuffer();
	virtual ~ForwardBuffer();

	POINTER Read(void *dest, POINTER bytes);
	bool Write(const void *from, POINTER bytes);
	POINTER GetSize(){
		return m_size;
	}
	POINTER GetCapacity(){
		return m_capacity;
	}
	void Grow(POINTER bytes);
	void Reset();
private:
	ForwardBuffer(const ForwardBuffer&);
	ForwardBuffer& operator=(const ForwardBuffer&);
private:
	void *m_buffer;
	POINTER m_size;
	POINTER m_pos;
	POINTER m_capacity;
	pthread_mutex_t *m_lock;
};

}
#endif
