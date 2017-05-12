//============================================================================
// Name        : tcpserver.cpp
// Author      : fangys
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;
#include "fyslib/sysutils.h"
#include "fyslib/pandc.hpp"
using namespace fyslib;
#include "tcpserver/tcpserver.h"
using namespace tcpserver;
#include <stdio.h>
#include <stdlib.h>


Pandc<string> *rq = NULL;
TcpServer *svr = NULL;

void ClientDisconnected(Client *client)
{
	rq->P(FormatString("client disconnected,%s\n",client->GetRemoteIP().c_str()));
}
void Startup()
{

}
void Shutdown()
{

}
void Error(int err_code, const char* err_msg)
{

}
void Recved(Client *c,void *buf,size_t len)
{
	void *bf = NULL;
	int l = len;
	if (SameText("status",string((char*)buf,6)))
	{
		rq->P(string("get server status"));
		string ret(svr->GetStatus());
		rq->P(ret);
		bf = malloc(ret.length());
		memcpy(bf,ret.c_str(),ret.length());
		l = ret.length();
	}
	else
	{
		rq->P(string((char*)buf,len));
		bf = malloc(len);
		memcpy(bf,buf,len);
		l = len;
	}
	c->Send(bf,l);
}
void Sent(Client *c,void *buf,size_t len,bool success )
{
	rq->P("sent");
	free(buf);
}

void ClientConnected(Client *client)
{
	client->m_on_recved = Recved;
	client->m_on_sent = Sent;

	rq->P(FormatString("client connected,%s\n",client->GetRemoteIP().c_str()));
}


int main() {
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	svr = new TcpServer((ExtractFilePath(ParamStr(0),true)+"tcpserver.ini").c_str(),NULL);
	svr->m_on_client_connected = ClientConnected;
	svr->m_on_client_disconnected = ClientDisconnected;
	if (!svr->Startup())
	{
		printf("startup ok\n");
		rq = new Pandc<string>;
		while(true)
		{
			printf("%s\n",rq->C().c_str());
		}
	}
	else
		printf("startup fail\n");

	return 0;
}
