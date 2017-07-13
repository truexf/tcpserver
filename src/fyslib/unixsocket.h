/*
 * unixsocket.h
 *
 *  Created on: Jul 13, 2017
 *      Author: root
 */

#ifndef FYSLIB_UNIXSOCKET_H_
#define FYSLIB_UNIXSOCKET_H_

#include "tthread.h"
namespace fyslib {

class UnixSocketClient;
typedef void (*OnUxSocketError)(int errCode);
typedef void (*OnUxSocketConnected)(UnixSocketClient *conn);

class UnixSocketClient
{
public:
    UnixSocketClient(): m_socket(-1) {
    }
    bool ConnectServer(const char* unixFn);
    static UnixSocketClient* Create(int fd);
    int Send(void *buf, int bufSize);
    int Recv(void *buf, int bufSize);
protected:
    void Run();
private:
    UnixSocketClient(const UnixSocketClient&);
    UnixSocketClient& operator = (const UnixSocketClient&);

    int m_socket;
};

class UnixSocketServer: public TThread
{
public:
    UnixSocketServer(): m_on_error(NULL), m_on_connected(NULL) {
    }
    void SetEventHandle(OnUxSocketError onErr, OnUxSocketConnected onConn){
        m_on_error = onErr;
        m_on_connected = onConn;
    }
    bool StartListen(const char* unixFn);
protected:
    void Run();
private:
    UnixSocketServer(const UnixSocketServer&);
    UnixSocketServer& operator = (const UnixSocketServer&);

    int m_socket;
    OnUxSocketError m_on_error;
    OnUxSocketConnected m_on_connected;
};

}


#endif /* FYSLIB_UNIXSOCKET_H_ */
