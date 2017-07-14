/*
 * unixsocket.h
 *
 *  Created on: Jul 13, 2017
 *      Author: root
 *      sample
 void OnUsConnected(UnixSocketClient *conn) {
    cout << "client connected" << endl;
    char sRecved[1024] = {0};
    for (;;) {
        int n = conn->Recv(sRecved, 1024);
        if (n <= 0) {
            cout<<"recv error: " << errno <<endl;
            break;
        }
        cout << "recved: " << string(sRecved,n) << endl;
        cout << "send: " << string(sRecved,n) << endl;
        if (conn->Send(sRecved,n) <= 0) {
            cout<<"send error: " << errno <<endl;
            break;
        }
    }
}
int main() {
    {
        string p(ParamStr(1));
        if (p == "c") {
            UnixSocketClient *c = new UnixSocketClient();
            if (c->ConnectServer("/tmp/unix.skt")) {
                char sRecv[1024] = {0};
                for(;;) {
                    char sGet[1024] = {0};
                    cin.getline(sGet,1023);
                    string s(sGet);
                    if (s == "exit" || s== "quit") {
                        break;
                    }
                    cout << "send: " << s << endl;
                    if (c->Send((void*)s.c_str(),s.length()) <= 0) {
                        cout << "send error:" << errno << endl;
                        break;
                    }
                    int n = c->Recv(sRecv,1024);
                    if (n == -1 || n == 0) {
                        cout << "recv error:" << errno << endl;
                        break;
                    }
                    cout << "recved: " << string(sRecv,n) << endl;
                }
            }
        } else if (p == "s"){
            UnixSocketServer *us = new UnixSocketServer();
            us->SetEventHandle(NULL,OnUsConnected);
            if (us->StartListen("/tmp/unix.skt")) {
                for(;;) {
                    string s;
                    cin >> s;
                    if (s == "exit") {
                        break;
                    }
                }
            }
        }
        return 0;
    }
}
 *
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
