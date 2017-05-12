/*
 * justice.h
 *
 *  Created on: Mar 16, 2016
 *      Author: root
struct 撸管者{
    姓名,
    次数
}
有n个撸管者。
设计相关数据结构和算法程序，简单高效的解决以下需求：
需求：撸
要求从取出次数最少的撸管者去撸，撸完次数+1
撸管者可能随时进入和离开。要保存他们撸的次数，下次进入时还原上次的次数。
 */

#ifndef FYSLIB_JUSTICE_H_
#define FYSLIB_JUSTICE_H_

#include <string>
#include <map>
using std::string;
using std::map;
#include "AutoObject.hpp"
#include "tthread.h"

namespace fyslib
{

typedef long long int64;

struct person
{
	string id;
	int64 grade;
	void *ptr;
};

struct personNode
{
	person *data;
	personNode *prev;
	personNode *next;
};

class justice
{
public:
	justice();
	virtual ~justice();

	void personIn(person *ps);
	person* personOut(string name);

	void saveToFile(string fn);
	void loadFromFile(string fn);

	person* fuck();
	void funcDone(person *ps);

	void free();
	void personDie(string name);
	person *personBorn(string name,int64 grade);
private:
	justice(const justice&);
	justice& operator=(const justice&);
private:
	map<string,personNode*> m_persons;
	map<string,person*> m_all_persons;
	personNode *head;
	personNode *tail;
	pthread_mutex_t *m_lock;
};

}
#endif /* FYSLIB_JUSTICE_H_ */
