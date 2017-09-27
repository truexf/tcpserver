/*
 * unixsocket.cpp
 *
 *  Created on: Jul 13, 2017
 *      Author: root
 */

#include "unixsocket.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "xtimer.h"

namespace fyslib{
bool UnixSocketClient::ConnectServer(const char* unixFn) {
    sockaddr_un addr;
    int fnLen = strlen(unixFn);
    if ((size_t)fnLen >= sizeof(addr.sun_path)) {
        return false;
    }
    m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == m_socket) {
        return false;
    }
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, unixFn, fnLen);
    int addrLen = sizeof(sockaddr_un);
    if (-1 == connect(m_socket, (const sockaddr*)&addr, addrLen)) {
        close(m_socket);
        return false;
    }
    return true;
}

int UnixSocketClient::Send(void *buf, int bufSize) {
    return send(m_socket,buf,bufSize,MSG_NOSIGNAL);
}

UnixSocketClient* UnixSocketClient::Create(int fd) {
    if (fd <= 0) {
        return NULL;
    }
    UnixSocketClient *ret = new UnixSocketClient;
    ret->m_socket = fd;
    return ret;
}

int UnixSocketClient::Recv(void *buf, int bufSize) {
    return recv(m_socket, buf, bufSize, 0);
}

bool UnixSocketServer::StartListen(const char* unixFn) {
    if (NULL == unixFn)
        return false;
    sockaddr_un addr;
    int fnLen = strlen(unixFn);
    if ((size_t)fnLen >= sizeof(addr.sun_path)) {
        return false;
    }
    unlink(unixFn);
    m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == m_socket) {
        return false;
    }
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, unixFn, fnLen);
    int addrLen = sizeof(sockaddr_un);
    if (-1 == bind(m_socket,(const sockaddr*)&addr, addrLen)) {
        close(m_socket);
        return false;
    }
    if (-1 == listen(m_socket,1024)) {
        close(m_socket);
        return false;
    }
    if (Start()) {
        return true;
    } else {
        return false;
    }
}

void UnixSocketServer::Run() {
    while (!GetTerminated()) {
        sockaddr_un addr;
        memset(&addr, 0, sizeof(sockaddr_un));
        addr.sun_family = AF_UNIX;
        unsigned int addrLen = sizeof(sockaddr_un);
        int skt = accept(m_socket,(sockaddr*)&addr, &addrLen);
        if (-1 == skt) {
            if (m_on_error) {
                m_on_error(errno);
                continue;
            }
        }
        fyslib::UnixSocketClient *client = fyslib::UnixSocketClient::Create(skt);
        if (m_on_connected) {
            //TThread *StartAsyncTimer(BaseFuncClass *proc,bool runfirst,const struct timespec *t);
            timespec ts;
            ts.tv_nsec = 0;
            ts.tv_sec = 1;
            AsyncExecute(AutoFunc(m_on_connected,client));
        }
    }
    close(m_socket);
}

}

