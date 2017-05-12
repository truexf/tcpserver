/*
 * types.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <unistd.h>
#include <sys/types.h>

namespace tcpserver{

class Listener;
class Client;
class Worker;
class Consumer;
class TcpServer;


typedef void (*OnClientConnected)(Client *client);
typedef void (*OnClientDisconnected)(Client *client);
typedef void (*OnStartup)();
typedef void (*OnShutdown)();
typedef void (*OnError)(int err_code, const char* err_msg);
typedef void (*OnRecved)(Client *c,void *buf,size_t len);
typedef void (*OnSent)(Client *c,void *buf,size_t len,bool success );

class Buf
{
public:
	void *m_buf;
	size_t m_len;
	Buf(void *buf,size_t len):m_buf(buf),m_len(len){
	}
	Buf():m_buf(NULL),m_len(0){}
};

class SendBuf
{
public:
	void *m_buf;
	size_t m_len;
	size_t m_pos;

	SendBuf(void *buf,size_t len,size_t pos):m_buf(buf),m_len(len),m_pos(pos){
	}
};

class RecvedBuf
{
public:
    void *m_buf;
    size_t m_len;
    RecvedBuf(void *bf, size_t len): m_buf(bf),m_len(len){
    }
};



}//end of namespace

#endif /* TYPES_HPP_ */
