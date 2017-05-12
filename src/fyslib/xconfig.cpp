/*
 * xconfig.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: root
 */

#include <vector>
#include "xconfig.h"
#include "sysutils.h"
#include "AutoObject.hpp"

using std::vector;

namespace fyslib{


XConfig::XConfig()
{
	m_lock = CreateMutex(false);
}

XConfig::~XConfig()
{
	Clear();
	DestroyMutex(m_lock);
}

void XConfig::Clear()
{
	for(map<string,map<string,string>* >::iterator it = m_data.begin();it!=m_data.end();++it)
	{
		delete it->second;
	}
	m_data.clear();
}

void XConfig::Reload()
{
	LoadFromFile(m_file);
}
bool XConfig::LoadFromString(const string cfg)
{
	if (cfg.empty())
		return false;
	AutoMutex auto1(m_lock);
	Clear();
	string s(cfg);
	vector<string> v;
	SplitString(s,"",v);
	for(size_t i=0;i<v.size();++i)
	{
		v[i] == trim(v[i]);
	}
	string sec,na,va;
	map<string,string> *map_sec = NULL;
	for(size_t i=0;i<v.size();++i)
	{
		string s = v[i];
		if (s.empty())
			continue;
		if('[' == s[0])
		{
			if(']' != s[s.length()-1])
				continue;
			map<string,string> *m = new map<string,string>;
			sec = s.substr(1,s.length()-2);
			map_sec = m;
			m_data[sec] = map_sec;
			continue;
		}
		else
		{
			if(NULL == map_sec)
				continue;
			string l,r;
			SplitString(s,"=",l,r);
			l = trim(l);
			r = trim(r);
			if(!l.empty())
				(*map_sec)[l] = r;
			continue;
		}
	}
	return true;
}
bool XConfig::LoadFromFile(const string &file)
{
	void *buf=NULL;
	size_t buf_size = 0;
	if (!LoadBufferFromFile(file,&buf,buf_size))
		return false;
	string s((char*)buf,buf_size);
	free(buf);
	if (LoadFromString(s)) {
		m_file = file;
		return true;
	} else {
		return false;
	}
}


bool XConfig::SaveToFile(const string &file)
{
	AutoMutex auto1(m_lock);
	vector<string> v;
	for(map<string,map<string,string>* >::iterator it = m_data.begin();it!=m_data.end();++it)
	{
		string sec("[");
		sec += it->first;
		sec += "]";
		v.push_back(sec);
		string s;
		for(map<string,string>::iterator it1 = it->second->begin(); it1!=it->second->end(); ++it1)
		{
			s.clear();
			s = s + it1->first + "=" + it1->second;
			v.push_back(s);
		}
	}
	string save = StringList2String(v,"\n");
	return SaveBufferToFile(file,(void*)save.c_str(),save.length());
}

string XConfig::Get(const string section,const string ident,string default_value)
{
	AutoMutex auto1(m_lock);
	map<string, map<string,string>* >::iterator it = m_data.find(section);
	if(m_data.end() == it)
		return default_value;
	map<string,string> *sec = it->second;
	map<string,string>::iterator it1 = sec->find(ident);
	if(sec->end() == it1)
		return default_value;
	return it1->second;
}

void XConfig::Set(const string section,const string ident,const string value)
{
	AutoMutex auto1(m_lock);
	map<string, map<string,string>* >::iterator it = m_data.find(section);
	if(m_data.end() == it)
	{
		map<string,string> *map_sec = new map<string,string>;
		m_data[section] = map_sec;
		(*map_sec)[ident] = value;
		return;
	}
	map<string,string> *sec = it->second;
	(*sec)[ident] = value;
}

int XConfig::GetInt(const string section,const string ident,int default_value)
{
	string s(this->Get(section,ident,""));
	if (s.empty())
		return default_value;
	else
	{
		return atoi(s.c_str());
	}
}

void XConfig::SetInt(const string section,const string ident,int value)
{
	string s = Int2Str(value);
	this->Set(section,ident,s);
}


}
