/*
 * tcpclient.h
 *
 *  Created on: Sep 4, 2017
 *      Author: root
 */

#ifndef TCPCLIENT_TCPCLIENT_H_
#define TCPCLIENT_TCPCLIENT_H_

/*  usage
void OnRecv(TcpClient *c,void *buf, size_t len)
{
    printf("%s\n",string((char*)buf,len).c_str());
}

int _tmain(int argc, _TCHAR* argv[])
{
    WSADATA wsa={0};
    WSAStartup(MAKEWORD(2,2),&wsa);
    AutoFuncClass auto1(AutoFunc(WSACleanup));

    XLog *log = new XLog();
    log->Start();
    TcpClientManager *m = new TcpClientManager;
    m->Init(log,10);
    m->Go();
    TcpClient *c = m->ConnectRemote("172.18.188.175",8888);
    if (c)
    {
        c->m_on_data_recved = OnRecv;
        string s;
        while(true)
        {
            getline(cin,s);
            if (SameText(s,"exit"))
                return 0;
            c->Send((void*)(s.c_str()),s.length());
        }
    }
    else
    {
        printf("start fail.");
    }
    return 0;
}
*/
#include "../fyslib/xlog.h"
#include <string>
#include <deque>
#include <vector>
#include <map>
#include <sys/epoll.h>
using std::string;
using std::deque;
using std::vector;
using std::map;
#include "../fyslib/tthread.h"
#include "../fyslib/fysbuffer.hpp"
#include "../fyslib/sysutils.h"
#include "../tcpclient/tcpclient.h"

using namespace fyslib;

class TcpClient;
class TcpClientManager;
class TcpClientWorker;

typedef void (*OnTcpClientConnected)(TcpClient *c);
typedef void (*OnTcpClientDisconnected)(TcpClient *c);
typedef void (*OnTcpClientDataSent)(TcpClient *c,void *buf, size_t len);
typedef void (*OnTcpClientDataRecved)(TcpClient *c,void *buf, size_t len);

struct TcpClientBuf
{
    TcpClient *m_client;
    void *m_buf;
    size_t m_len;
};

struct ClientSendBuf
{
    void *m_buf;
    size_t m_len;
    size_t m_send_pos;
    ClientSendBuf(void *buf,size_t len,size_t pos): m_buf(buf),m_len(len),m_send_pos(pos){}
};

struct ClosedTcpClient
{
    SYSTEMTIME m_close_time;
    TcpClient *m_client;
};

class TcpClientManager: public TThread
{
    friend class TcpClient;
    friend class TcpClientWorker;
private:
    int m_epoll_fd;
    map<string, TcpClient*> m_clients;
    pthread_mutex_t *m_clients_lock;
    deque<TcpClientWorker*> m_workers;
    pthread_mutex_t *m_workers_lock;
    pthread_mutex_t *m_consumers_lock;
    XLog *m_log;
    HBuff *m_perhandledata_buf;
    int m_worker_count;
    map<string, ClosedTcpClient> m_closed_clients;
    pthread_mutex_t *m_closed_clients_lock;
protected:
    void Run();
public:
    OnTcpClientConnected m_on_client_connected;
    OnTcpClientDisconnected m_on_client_disconnected;

    void Init(XLog *log, int worker_count);
    bool Go();
    TcpClient *ConnectRemote(timeval *tv, string ip, unsigned short port, OnTcpClientDataRecved onRecved, OnTcpClientDataSent onSent, void *setData);
    void RemoveClient(TcpClient *c);
};

class TcpClient
{
    friend class TcpClientManager;
    friend class TcpClientWorker;
private:
    int m_socket;
    string m_remote_ip;
    unsigned short m_remote_port;
    deque<ClientSendBuf> m_send_queue;
    pthread_mutex_t *m_send_queue_lock;
    TcpClientManager *m_mgr;
    char m_recv_buf[8192];
    pthread_mutex_t *m_recv_lock;
    bool m_close_mark;

    bool Recv();
    void SetSocket(int s){m_socket = s;}
public:
    string m_uuid;
    void *m_data;

    OnTcpClientDataRecved m_on_data_recved;
    OnTcpClientDataSent m_on_data_sent;

    TcpClient();
    virtual ~TcpClient();
    void SetMgr(TcpClientManager *m){m_mgr = m;}
    void SetData(void *data){m_data = data;}
    bool Send(void *buf,size_t len);
};

class TcpClientWorker: public TThread
{
private:
    TcpClientManager *m_mgr;
    struct epoll_event m_evlist[24];
protected:
    void Run();
public:
    void SetMgr(TcpClientManager *m){m_mgr = m;}
};


#endif /* TCPCLIENT_TCPCLIENT_H_ */
