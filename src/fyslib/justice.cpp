/*
 * justice.cpp
 *
 *  Created on: Mar 16, 2016
 *      Author: root
 */

#include "justice.h"
#include "sysutils.h"
#include <vector>
using std::vector;

namespace fyslib
{


justice::justice()
{
	head = NULL;
	tail = NULL;
	m_lock = CreateMutex(true);
}

justice::~justice()
{
	{
		AutoMutex mtx(m_lock);
		personNode *n = head;
		while(n)
		{
			personNode *next = n->next;
			delete (personNode*)n;
			n = next;
		}
	}
	DestroyMutex(m_lock);
}

void justice::personIn(person* ps)
{
	if (!ps)
		return;
	AutoMutex mtx1(m_lock);
	personNode *nodeNew = new personNode;
	nodeNew->data = ps;
	if(!head)
	{
		head = nodeNew;
		tail = nodeNew;
		nodeNew->prev = NULL;
		nodeNew->next = NULL;
	}
	else
	{
		personNode *n = head;
		bool added = false;
		while (n)
		{
			if (n->data->grade >= ps->grade)
			{
				if(n->prev)
				{
					nodeNew->prev = n->prev;
					nodeNew->next = n;
					n->prev->next = nodeNew;
					n->prev = nodeNew;
				}
				else
				{
					nodeNew->prev = NULL;
					nodeNew->next = n;
					n->prev = nodeNew;
					head = nodeNew;
				}
				added = true;
				break;
			}
			n = n->next;
		}
		if (!added)
		{
			nodeNew->prev = tail;
			nodeNew->next = NULL;
			if(tail)
				tail->next = nodeNew;
			tail = nodeNew;
		}
	}
	m_persons[ps->id] = nodeNew;
}

person* justice::personOut(string name)
{
	AutoMutex mtx1(m_lock);
	map<string,personNode*>::iterator it = m_persons.find(name);
	if (it == m_persons.end())
		return NULL;
	personNode *node = it->second;
	if (node->prev)
	{
		node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
	}
	else
	{
		if (node->next)
		{
			head = node->next;
			head->prev = NULL;
		}
		else
		{
			head = NULL;
			tail = NULL;
		}
	}
	person *ret = node->data;
	delete node;
	m_persons.erase(it);
	return ret;
}

void justice::saveToFile(string fn)
{
	AutoMutex mtx(m_lock);
	MemoryStream mem;
	for(map<string,person*>::iterator it = m_all_persons.begin(); it!=m_all_persons.end();it++)
	{
		mem.WriteString(FormatString("%s:%lld\n",it->first.c_str(),it->second->grade));
	}
	mem.SaveToFile(fn);
}

void justice::loadFromFile(string fn)
{
	AutoMutex mtx(m_lock);
	void *buf = NULL;
	size_t bufLen = 0;
	if (!LoadBufferFromFile(fn,&buf,bufLen))
		return;
	string s((char*)buf,bufLen);
	vector<string> v;
	SplitString(s,"\n",v);
	for(vector<string>::iterator it = v.begin(); it!=v.end(); it++)
	{
		string l,r;
		SplitString(*it,":",l,r);
		if (l.empty() || r.empty())
			continue;
		personBorn(l,atoll(r.c_str()));
	}
}

person* justice::fuck()
{
	AutoMutex mtx(m_lock);
	if(!head)
		return NULL;
	personNode *n = head;
	if(head->next)
		head->next->prev = NULL;
	head = head->next;
	person *ret = n->data;
	m_persons.erase(ret->id);
	delete (personNode*)n;
	return ret;
}

void justice::funcDone(person* ps)
{
	if (!ps)
		return;
	AutoMutex mtx(m_lock);
	if (m_persons.find(ps->id) != m_persons.end())
		return;
	ps->grade += 1;
	personNode *newNode = new personNode;
	newNode->data = ps;
	newNode->next = NULL;
	personNode *t = tail;
	bool added = false;
	while(t)
	{
		if(t->data->grade <= ps->grade)
		{
			newNode->prev = t;
			if (t->next)
				t->next->prev = newNode;
			newNode->next = t->next;
			t->next = newNode;
			added = true;
			break;
		}
		t = t->prev;
	}
	if (!added)
	{
		if(head)
		{
			head->prev = newNode;
			newNode->prev = NULL;
			newNode->next = head;
			head = newNode;
		}
		else
		{
			head = newNode;
			tail = newNode;
			head->prev = NULL;
			head->next = NULL;
			tail->prev = NULL;
			tail->next = NULL;
		}
	}
	m_persons[ps->id] = newNode;
}

void justice::personDie(string name)
{
	AutoMutex mtx(m_lock);
	person *ps = personOut(name);
	m_all_persons.erase(name);
	delete ps;
}

person* justice::personBorn(string name,int64 grade)
{
	AutoMutex mtx(m_lock);
	map<string,person*>::iterator it = m_all_persons.find(name);
	if (it!=m_all_persons.end())
		return it->second;
	person* ret = new person;
	ret->id = name;
	ret->grade = grade;
	ret->ptr = NULL;
	m_all_persons[name] = ret;
	return ret;
}

}
