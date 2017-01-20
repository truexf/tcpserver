/*
 * bs.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: root
 */

#include "bs.h"

#include "tcpserver/tcpserver.h"
using tcpserver::Client;
using tcpserver::TcpServer;
#include <sys/socket.h>

BServer *_bserver = NULL;

void BClientDisconnected(Client *client)
{
	if (client->m_data)
	{
		BClientData *d = (BClientData*)client->m_data;
		_bserver->RemovePoolClient(client);
		_bserver->PushClientData(d);
		if (d->m_client_a)
			close(d->m_client_a->m_socket);
	}
}
void BStartup()
{

}
void BShutdown()
{

}
void BError(int err_code, const char* err_msg)
{

}
void BRecved(Client *c,void *buf,size_t len)
{
	if (c->m_data && buf && len > 0)
	{
		BClientData* d = (BClientData*)c->m_data;
		if (d->m_state == bcsStandby && d->m_client_a)
		{
			void *bf = malloc(len);
			memcpy(bf,buf,len);
			d->m_client_a->Send(bf,len);
		}
		else
		{
			d->m_mac.assign((char*)buf,len);
			d->m_state = bcsStandby;
			_bserver->PushFreeClient(c);
		}
	}
}
void BSent(Client *c,void *buf,size_t len,bool success )
{
	if (buf && len > 0)
		free(buf);
}

void BClientConnected(Client *client)
{
	BClientData *d = _bserver->PullClientData();
	client->SetData(d);
}

bool StartBS(XLog *log)
{
	_bserver = new BServer;
	return _bserver->Start(log);
}

BServer::BServer()
{
	m_client_data_pool = new ObjectPool<BClientData>;
	m_client_pool_lock = CreateMutex(false);
}

BServer::~BServer() {

}

bool BServer::Start(XLog *log)
{
  //todo
}

void BServer::PushFreeClient(Client *c)
{
	if (!c)
		return;
	AutoMutex auto1(m_client_pool_lock);
	BClientData *d = (BClientData*)c->m_data;
	d->m_client_a = NULL;
	map<string, vector<Client*>* >::iterator it = m_client_pool.find(d->m_mac);
	if (it != m_client_pool.end())
	{
		(it->second)->push_back(c);
	}
	else
	{
		vector<Client*> *cs = new vector<Client*>;
		cs->push_back(c);
		m_client_pool[d->m_mac] = cs;
	}
}
Client* BServer::PullFreeClient(string mac)
{
	AutoMutex auto1(m_client_pool_lock);
	map<string, vector<Client*>* >::iterator it = m_client_pool.find(mac);
	if (it != m_client_pool.end())
	{
		if (!it->second->empty())
		{
			Client *ret = it->second->back();
			it->second->pop_back();
			return ret;
		}
		else
			return NULL;
	}
	else
		return NULL;
}
void BServer::GetAllClientMac(vector<string> &ret)
{
	AutoMutex auto1(m_client_pool_lock);
	for (map<string, vector<Client*>* >::iterator it = m_client_pool.begin(); it != m_client_pool.end(); it++)
		ret.push_back(it->first);
	return;
}
void BServer::PushClientData(BClientData *d)
{
	m_client_data_pool->PushObject(d);
}
BClientData* BServer::PullClientData()
{
	BClientData* d = m_client_data_pool->PullObject();
	if (!d)
		d = new BClientData();
	d->m_mac.clear();
	d->m_state = bcsConnected;
	d->m_client_a = NULL;
	return d;
}
void BServer:: RemovePoolClient(Client *c)
{
	if (!c || !c->m_data)
		return;
	BClientData *d = (BClientData*)c->m_data;
	if (d->m_mac.empty())
		return;
	AutoMutex auto1(m_client_pool_lock);
	map<string, vector<Client*>* >::iterator it = m_client_pool.find(d->m_mac);
	if (it != m_client_pool.end())
	{
		for (vector<Client*>::iterator itr = it->second->begin(); itr != it->second->end(); itr++)
		{
			if (*itr == c)
			{
				it->second->erase(itr);
			}
		}
	}
}
