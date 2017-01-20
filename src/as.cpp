/*
 * as.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: root
 */

#include "as.h"
#include "tcpserver/tcpserver.h"
using namespace tcpserver;
#include "bs.h"
#include "fyslib/sysutils.h"
#include "fyslib/tthread.h"
#include "fyslib/xsocketutils.hpp"
using namespace fyslib;

AServer *_aserver = NULL;

void AClientDisconnected(Client *client)
{
	if (!client || !client->m_data || !_bserver)
		return;
	_bserver->PushFreeClient(client);
}
void AStartup()
{

}
void AShutdown()
{

}
void AError(int err_code, const char* err_msg)
{

}
void ARecved(Client *c,void *buf,size_t len)
{
	if (!c || !buf || 0 == len)
		return;
	void *bf = malloc(len);
	if (!bf)
	{
		LOG_ERR(_aserver->m_log,FormatString("malloc fail,%d",errno).c_str());
		return;
	}
	memcpy(bf,buf,len);
	Client *b = (Client*)c->m_data;
	b->Send(bf,len);
}
void ASent(Client *c,void *buf,size_t len,bool success )
{
	if (buf)
		free(buf);
}

void AClientConnected(Client *client)
{
	if (!client || !_bserver)
		return;
	string mac = _aserver->GetMacByPort(client->m_listen_port);
	Client *b = _bserver->PullFreeClient(mac);
	if (!b)
		close(client->m_socket);
	else
		client->SetData(b);

	return;
}

bool StartAS(XLog *log)
{
	_aserver = new AServer;
	return _aserver->Start(log);
}

AServer::AServer()
{
	m_port_macs_lock = CreateMutex(false);
}

AServer::~AServer()
{

}

string AServer::GetPortMacList()
{
	AutoMutex auto1(m_port_macs_lock);
	string ret = "";
	for (map<ushort,string>::iterator it = m_port_macs.begin(); it != m_port_macs.end(); it++)
	{
		if (ret.empty())
			ret = FormatString("%d=%s",it->first,it->second);
		else
			ret = ret + FormatString("&%d=%s",it->first,it->second);
	}
	return ret;
}

bool AServer::Start(XLog *log)
{
	m_log = log;
	m_svr = new TcpServer((ExtractFilePath(ParamStr(0),true)+"tcpserver.ini").c_str(),m_log);
	m_svr->m_on_client_connected = AClientConnected;
	m_svr->m_on_client_disconnected = AClientDisconnected;
	if (!m_svr->Startup())
		return false;
	for (int i = 0; i < 50; i++)
	{
		Listener* lsn = m_svr->StartListener(0);
		if (lsn)
			m_port_macs[lsn->GetListenPort()] = "";
		else
			return false;
	}
}

void AServer::BindPortMac(ushort port, string mac)
{
	AutoMutex auto1(m_port_macs_lock);
	map<ushort,string>::iterator it = m_port_macs.find(port);
	if (it == m_port_macs.end())
		return;
	m_port_macs[port] = mac;
}

ushort AServer::GetPortByMac(string mac)
{
	AutoMutex auto1(m_port_macs_lock);
	for (map<ushort,string>::iterator it = m_port_macs.begin(); it != m_port_macs.end(); it++)
	{
		if (it->second == mac)
		{
			return it->first;
		}
	}
	return 0;
}

string AServer::GetMacByPort(ushort port)
{
	AutoMutex auto1(m_port_macs_lock);
	map<ushort,string>::iterator it = m_port_macs.find(port);
	if (it != m_port_macs.end())
		return it->second;
	return "";
}


