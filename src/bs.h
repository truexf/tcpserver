/*
 * bs.h
 *
 *  Created on: Jan 6, 2017
 *      Author: root
 */

#ifndef BS_H_
#define BS_H_

#include "tcpserver/tcpserver.h"
#include "fyslib/objectpool.hpp"
#include "fyslib/tthread.h"
using namespace fyslib;
using namespace tcpserver;
#include <vector>
#include <map>
using std::vector;
using std::map;

bool StartBS(XLog *log);

enum BClientState
{
	bcsConnected,
	bcsStandby
};

class BClientData
{
public:
	Client *m_client_a;
	BClientState m_state;
	string m_mac;

	BClientData():m_client_a(NULL),m_state(bcsConnected){}
};

class BServer
{
private:
	BServer(const BServer&);
	BServer& operator = (const BServer&);

	map<string,vector<Client*>* > m_client_pool; //key is mac
	pthread_mutex_t *m_client_pool_lock;
	ObjectPool<BClientData> *m_client_data_pool;
public:
	BServer();
	~BServer();

	bool Start(XLog *log);

	void PushFreeClient(Client *c);
	Client* PullFreeClient(string mac);
	void GetAllClientMac(vector<string> &ret);
	void PushClientData(BClientData *d);
	BClientData* PullClientData();
	void RemovePoolClient(Client *c);
};

extern BServer *_bserver;




#endif /* BS_H_ */
