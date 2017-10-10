/*
 * server.h
 *
 *  Created on: Apr 12, 2017
 *      Author: root
 */

#ifndef PROXYSERVER_H_
#define PROXYSERVER_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include "fyslib/tthread.h"
#include "fyslib/sysutils.h"
#include "fyslib/xlog.h"
#include <uuid/uuid.h>

#include "fyslib/obj_pool.h"
#include "tcpserver/tcpserver.h"
#include "fyslib/systemtime.h"
#include "fyslib/xlog.h"
using namespace tcpserver;
using namespace std;
using namespace fyslib;
/*  server端流程
1. 启动，侦听3个端口,用于下列服务
2. 侦听并接受cproxy的连接，并接收数据，若是c:mac\n,认为是控制连接，push到cproxy-list。若是t:uuid\n，则判断该数据是否之前发出的t:uuid\n指令，若不是，断开连接，若是则参考ps的流程。
3. 提供cproxy-list接口供wb查询，返回格式如下：
    mac1\n
    mac2\n
    mac3\n
    ...
    macn\n\n
4. 提供getcproxyservice接口，请求格式为： mac\n 响应格式为 port\n
    getcproxyservice接口收到请求后，根据请求中指定的mac，以一个新端口建立一个proxyserver(ps),并与该mac关联。ps进入侦听，并把侦听的端口号响应回去，wb收到端口号以后，以http代理方式连接ps并进行数据传输
proxyserver(ps)的流程：
1. 收到wb的请求，解析请求头，判断是普通代理还是隧道代理。生成一个与标识该wb连接的uuid,并通知server向cproxy发起数据传输指令 t:uuid\n
2. cp收到指令后建立一个新的到server的连接(cp)，并写回 t:uuid\n。server判断uuid与发出时一致，通知ps连接成功，ps根据代理方式请server向cproxy端连接发送指令http|https:host:port\n
3. cp收到指令后，根据指令中的host和port连接远端网站，连接成功后给bsserver发送ok\n,server收到后通知ps可以开始传输数据了。
4. ps经过cp进行数据转发
*/
extern XLog *_logger;
enum CProxyClientState {
    ccsNew,//cctUnknow
    ccsConnected, //cctUnknow
    ccsMacRecved, //cctControl
    ccsUUIDRecved //cctTransfer
};

enum CProxyClientType {
    cctUnknown,
    cctControl,
    cctTransfer
};

enum WbClientType {
    wctUnknown,
    wctHttp,
    wctTunnel
};

class PooledHttpRequest;
class WbClient;
class ProxyServer;
class CProxyClient: public PoolObject {
public:
    string m_id;
    CProxyClientState m_state;
    CProxyClientType m_type;
    string m_mac;
    Client *m_tcp_client;
    WbClient *m_wb_client;
    PooledHttpRequest *m_http_request;
    char m_packet_buf[8192];
    size_t m_packet_buf_off;
    SYSTEMTIME m_last_time;
    string m_wb_uuid;

    CProxyClient() {
        Init();
    }
    const char* AsString(){
        return m_id.c_str();
    }

    void Init() {
        m_id = CreateGUID();
        m_wb_uuid.clear();
        m_state = ccsNew;
        m_type = cctUnknown;
        m_tcp_client = NULL;
        m_wb_client = NULL;
        m_http_request = NULL;
        memset(m_packet_buf,0,sizeof(m_packet_buf));
        m_packet_buf_off = 0;
    }

    void Fina();
};

class ProxyServer;

enum WbClientState {
    wcsNew,
    wcsConnected,
    wcsTunnelCreated
};

struct BufBlock {
    void *buf;
    size_t len;
    BufBlock(void *b,size_t l): buf(b), len(l) {
    }
};

class WbClient: public PoolObject {
public:
    string m_id;
    Client *m_tcp_client;
    CProxyClient *m_tunnel_client;
    WbClientType m_type;
    WbClientState m_state;
    PooledHttpRequest *m_current_request;
    char m_packet_buf[16384];
    int m_packet_buf_off;
    string m_host;
    vector<PooledHttpRequest*> m_requests;
    ProxyServer *m_proxy_server;
    SYSTEMTIME m_last_time;
    deque<BufBlock> m_cached_bufs; //for tunnel wb buf cache
    void PushBuf(void *buf, size_t len);
    void SendCache();
    WbClient() {
        Init();
    }
    ~WbClient() {
    }

    const char* AsString();

    void Init() {
        m_id = CreateGUID();
        m_tcp_client = NULL;
        m_tunnel_client = NULL;
        m_type = wctUnknown;
        m_state = wcsNew;
        m_current_request = NULL;
        m_packet_buf_off = 0;
        memset(m_packet_buf, 0, sizeof(m_packet_buf));
        m_host.clear();
        m_requests.clear();
        m_cached_bufs.clear();
    }
    void Fina();
};

class PooledHttpRequest: public PoolObject {
public:
    WbClient *m_wb;
    CProxyClient *m_cp_client;
    string m_http_proxy_request;
    string m_host;
    string m_path;
    int m_content_length;
    string m_body;
    bool m_data_completed;
    bool m_remote_connected;
    deque<BufBlock> m_cached_bufs;
    PooledHttpRequest() {
        Init();
    }
    ~PooledHttpRequest() {
    }
    void PushBuf(void *buf, size_t len);
    void SendCache();

    void Init() {
        m_wb = NULL;
        m_cp_client = NULL;
        m_http_proxy_request.clear();
        m_body.clear();
        m_host.clear();
        m_path.clear();
        m_content_length = 0;
        m_remote_connected = false;
        m_data_completed = false;
        m_cached_bufs.clear();
    }
    void Fina();

};

class ProxyServer: public TThread {
protected:
    void Run();
public:
    TcpServer *m_tcp_server;
    string m_mac;
    string m_port;
    XLog *m_log;

    ProxyServer();
    ~ProxyServer();
    void SetLogger(XLog *log) {
        m_log = log;
    }

    bool StartListen();
};

struct UUIDInfo {
    //TCondition *cond;
    CProxyClient *cpTransfer;
    WbClient *wb;
    PooledHttpRequest *wbRequest;
    string hostData;

    UUIDInfo() {
        //cond = new TCondition();
        cpTransfer = NULL;
        wb = NULL;
        wbRequest = NULL;
    }
    ~UUIDInfo() {
        //delete cond;
    }

//    bool Wait() {
//        timespec t;
//        t.tv_nsec = 0;
//        t.tv_sec = 5;
//        return cond->Wait(&t) == 0;
//    }
};

class AServer: public TThread {
private:
    friend void OnClientConnected_cproxy(Client *client);
    friend void OnClientDisconnected_cproxy(Client *client);
    friend void OnRecved_cproxy(Client *c,void *buf,size_t len);
    friend void OnSent_cproxy(Client *c,void *buf,size_t len,bool success );

    friend void OnClientConnected_as(Client *client);
    friend void OnClientDisconnected_as(Client *client);
    friend void OnRecved_as(Client *c,void *buf,size_t len);
    friend void OnSent_as(Client *c,void *buf,size_t len,bool success );

    friend void OnClientConnected_wb(Client *client);
    friend void OnClientDisconnected_wb(Client *client);
    friend void OnRecved_wb(Client *c,void *buf,size_t len);
    friend void OnSent_wb(Client *c,void *buf,size_t len,bool success );

    XLog *m_log;
    int m_max_proxy;
    map<CProxyClient*, SYSTEMTIME> m_born_cproxy_clients; //tcpclient:born-time
    pthread_mutex_t *m_born_cproxy_clients_lock;
    map<CProxyClient*, SYSTEMTIME> m_dead_cproxy_clients;
    set<CProxyClient*> m_cproxy_clients;
    pthread_mutex_t *m_cproxy_clients_lock;

    map<string, CProxyClient*> m_cp_list;
    map<string, WbClient*> m_wb_list;
    pthread_mutex_t *m_cp_list_lock;
    pthread_mutex_t *m_wb_list_lock;

    map<Client*, SYSTEMTIME> m_as_clients;
    pthread_mutex_t *m_as_clients_lock;

    TcpServer *m_tcp_server;
    TcpServer *m_tcp_server_as;
    map<string, ProxyServer*> m_proxys;  //mac:ps
    vector<ProxyServer*> m_free_proxys;
    pthread_mutex_t *m_proxys_lock;

    map<string, UUIDInfo*> m_uuid_list;
    pthread_mutex_t *m_uuid_list_lock;
public:
    ObjPool *m_http_request_pool;
    ObjPool *m_wb_pool;
    ObjPool *m_cp_pool; //for transfer cp
public:
    AServer();
    ~AServer();
    void FreeProxyService(string mac);
    string GetCProxyList();
    string GetProxyService(string mac);
    string ReplaceProxyServiceMac(string oldMac, string newMac);
    string GetProxyList();
    ProxyServer* CreateProxyService(CProxyClient* cp);
    void RemoveProxyService(ProxyServer *ps);
    void RemoveProxyService(string mac);
    void AddCProxyClient(CProxyClient* clt);
    void RemoveCProxyClient(CProxyClient *clt);
    CProxyClient* FindCProxy(string mac);
    void SetLogger(XLog *log) {
        m_log = log;
        _logger = log;
    }
    void SetMaxProxy(int v) {
        m_max_proxy = v;
    }
    bool StartListen();

    UUIDInfo* FindUUID(string uuid);
    void AddUUID(string uuid, UUIDInfo* cp);
    void RemoveUUID(string uuid);
    void CreateTransferClient(WbClient *wb, PooledHttpRequest *req, string hostData, string mac);
    void RemoveWb(string id);
    void RemoveCp(string id);
    void AddWb(WbClient* wb);
    void AddCp(CProxyClient* cp);
protected:
    void Run();
};
extern AServer *_aserver;


#define XDEBUG
#define WARN(x) LOG_WARN(_logger, x)
#define INFO(x) LOG_INFO(_logger, x)
#define ERR(x) LOG_ERR(_logger, x)
#ifdef XDEBUG
#define debug_str(x) LOG_INFO(_logger, x)
#else
#define debug_str(x) LOG_INFO(_logger, x)
#endif

inline void* MyMalloc(int len) {
    void *ret = malloc(len);
    if (ret) {
        debug_str(Fmt("malloc %x, len %d\n",(long long)ret, len).c_str());
    } else {
        debug_str(Fmt("malloc fail, len %d\n", len).c_str());
    }
    return ret;
}

inline void MyFree(void *v) {
    if (v) {
        debug_str(Fmt("free %x \n",(long long)v).c_str());
        free(v);
    }
}


void OnError(int err_code, const char* err_msg);

void OnClientConnected_cproxy(Client *client);
void OnClientDisconnected_cproxy(Client *client);
void OnRecved_cproxy(Client *c,void *buf,size_t len);
void OnSent_cproxy(Client *c,void *buf,size_t len,bool success );

void OnClientConnected_wb(Client *client);
void OnClientDisconnected_wb(Client *client);
void OnRecved_wb(Client *c,void *buf,size_t len);
void OnSent_wb(Client *c,void *buf,size_t len,bool success );

void OnClientConnected_as(Client *client);
void OnClientDisconnected_as(Client *client);
void OnRecved_as(Client *c,void *buf,size_t len);
void OnSent_as(Client *c, void *buf, size_t len, bool success);


#endif /* PROXYSERVER_H_ */
