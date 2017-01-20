/*
 * clientconnection.h
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>
#include <map>
#include <deque>
#include "../fyslib/objectpool.hpp"
#include "../fyslib/systemtime.h"
#include "../fyslib/tthread.h"
using std::string;
using std::map;
using namespace fyslib;
#include "types.hpp"
#include <sys/types.h>

namespace tcpserver{
class TcpServer;
class Client
{
private:
	Client(const Client&);
	Client& operator = (const Client&);
public:
	//Client():m_socket(-1),m_data(NULL),m_close_mark(false),m_on_recved(NULL),m_on_sent(NULL),m_send_queue_lock(NULL){}
	Client(int socket_fd,TcpServer *svr,ushort listen_port);
	~Client();
	int m_socket;
	string m_uuid;
	void *m_data;
	bool m_close_mark;
	ushort m_listen_port;
	TcpServer *m_svr;

	OnRecved m_on_recved;
	OnSent m_on_sent;

	void Init(int socket_fd, ushort listen_port);
	bool Send(void *buf,size_t len);
	void ClearSendQueue();
	string GetRemoteIP();
	void SetData(void *ptr){
		m_data = ptr;
	}
private:
	deque<SendBuf> m_send_queue;
	pthread_mutex_t *m_send_queue_lock;
};

class ClosedClient
{
public:
	explicit ClosedClient(Client *c):m_client(c){
		GetLocalTime(&m_close_time);
	}
	ClosedClient():m_client(NULL){}
	Client *m_client;
	SYSTEMTIME m_close_time;
};

class ClientManager: public TThread
{
private:
	ClientManager(const ClientManager&);
	ClientManager& operator = (const ClientManager&);

	ObjectPool<Client> *m_client_pool;
	map<string, ClosedClient> m_recycling_pool;
	pthread_mutex_t *m_lock;
	TcpServer *m_svr;
protected:
	void Run();
public:
	ClientManager(TcpServer *svr);
	~ClientManager();

	void CloseClient(Client *c);
	Client* NewClient(int socket_fd, ushort listen_port);

	int PoolSize();
	int RecyclingSize();
};

struct ClientBuf
{
	Client *m_client;
	void *m_buf;
	size_t m_len;

	ClientBuf(Client *c, void *buf, size_t len):m_client(c){
		m_buf = buf;
		m_len = len;
	}
};

}



#endif /* CLIENT_H_ */
