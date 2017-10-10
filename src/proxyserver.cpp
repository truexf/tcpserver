/*
 *
 *  Created on: Apr 12, 2017
 *      Author: root
 */
#include "proxyserver.h"
#include "fyslib/tthread.h"
#include "fyslib/AutoObject.hpp"
#include "fyslib/xtimer.h"
#include "fyslib/sysutils.h"
#include "tcpserver/tcpserver.h"
#include "fyslib/xlog.h"
#include <string.h>
using namespace fyslib;

XLog *_logger = NULL;
ProxyServer::ProxyServer() {
    m_tcp_server = NULL;
    m_log = NULL;
}

ProxyServer::~ProxyServer() {

}

const char * const _proxy_cfg = "[SERVER]\n\
heartbeat=1\n\
heartbeat_interval_second=30\n\
linger=0\n\
listenqueue_length=50\n\
sendbuf_size=-1\n\
recvbuf_szie=-1\n\
bind_ip=0.0.0.0\n\
listen_port=0\n\
log_path=\n\
worker_count=5\n\
consumer_count=3";
bool ProxyServer::StartListen() {
    debug_str("ProxyServer::StartListen()");
    string scfg(LoadStringFromFile(ExtractFilePath(ParamStr(0),true)+"proxy.ini"));
    if (scfg.empty()) {
        scfg.assign(_proxy_cfg);
    }
    INFO(Fmt("proxy config:\n%s",scfg.c_str()).c_str());
    m_tcp_server = new TcpServer(scfg, m_log);
    m_tcp_server->m_on_client_connected = OnClientConnected_wb;
    m_tcp_server->m_on_client_disconnected = OnClientDisconnected_wb;
    if (!m_tcp_server->Startup()) {
        ERR("AServer::CreateProxyService,tcp server Startup fail.");
        return false;
    }
    m_port = FormatString("%d",m_tcp_server->GetListenPort());
    return true;
}

void ProxyServer::Run() {
    debug_str("ProxyServer::Run()");
    while (!GetTerminated()) {
        timespec ts;
        ts.tv_nsec = 0;
        ts.tv_sec = 5;
        SleepExactly(&ts);
    }
    debug_str(Fmt("proxy %s shutdown",m_port.c_str()).c_str());
    if (m_tcp_server)
        m_tcp_server->Shutdown(true);
}

AServer *_aserver = NULL;
AServer::AServer() {
    m_log = NULL;
    m_max_proxy = 100;
    m_cproxy_clients_lock = CreateMutex(true);
    m_born_cproxy_clients_lock = CreateMutex(true);
    m_as_clients_lock = CreateMutex(true);
    m_tcp_server = NULL;
    m_proxys_lock = CreateMutex(true);
    m_uuid_list_lock = CreateMutex(true);
    m_cp_list_lock = CreateMutex(true);
    m_wb_list_lock = CreateMutex(true);
    m_tcp_server = NULL;
    m_tcp_server_as = NULL;
    m_http_request_pool = new ObjPool;
    m_cp_pool = new ObjPool;
    m_wb_pool = new ObjPool;
}
AServer::~AServer() {

}
void AServer::Run() {
    debug_str("AServer::Run()");
    _aserver = this;
    if (!StartListen()) {
        ERR("StartListen fail.");
        return;
    }
    debug_str("Aserver StartListen success.");
    while (!GetTerminated()) {
        timespec ts;
        ts.tv_nsec = 0;
        ts.tv_sec = 3;
        SleepExactly(&ts);

        //clean timeout as clients
        {
            //debug_str("clean timeout as clients");
            vector<Client*> clients;
            SYSTEMTIME st;
            GetLocalTime(&st);
            {
                AutoMutex auto1(m_as_clients_lock);
                for (map<Client*, SYSTEMTIME>::iterator it = m_as_clients.begin(); it != m_as_clients.end(); it++) {
                    if (SecondsBetween(it->second, st) > 30) {
                        clients.push_back(it->first);
                    }
                }
                for (size_t i=0;i<clients.size();i++) {
                    debug_str("clean as client");
                    m_as_clients.erase(clients[i]);
                }
            }
            for (size_t i=0;i<clients.size();i++) {
                clients[i]->Close();
            }
        }

        //clean timeout born tcpclients
        {
            //debug_str("clean timeout born clients");
            vector<CProxyClient*> clients;
            SYSTEMTIME st;
            GetLocalTime(&st);
            {
                AutoMutex auto1(m_born_cproxy_clients_lock);
                for (map<CProxyClient*, SYSTEMTIME>::iterator it = m_born_cproxy_clients.begin(); it != m_born_cproxy_clients.end(); it++) {
                    if (SecondsBetween(it->second, st) > 30) {
                        clients.push_back(it->first);
                    }
                }
                for (vector<CProxyClient*>::iterator it = clients.begin(); it != clients.end(); it++) {
                    debug_str(Fmt("clean cp %s",(*it)->AsString()).c_str());
                    (*it)->m_tcp_client->SetData(NULL);
                    (*it)->m_tcp_client->Close();
                    m_born_cproxy_clients.erase(*it);
                }
            }
            for (vector<CProxyClient*>::iterator it = clients.begin(); it != clients.end(); it++) {
                delete (*it);
            }
        }

        //clean dead cproxy clients
        {
            //debug_str("clean dead cproxy clients.");
            vector<CProxyClient*> toDelete;
            SYSTEMTIME st;
            GetLocalTime(&st);
            {
                AutoMutex auto1(m_cproxy_clients_lock);
                for (map<CProxyClient*, SYSTEMTIME>::iterator it = m_dead_cproxy_clients.begin(); it != m_dead_cproxy_clients.end(); it++) {
                    if (SecondsBetween(it->second, st) > 30) {
                        toDelete.push_back(it->first);
                    }
                }
                for(size_t i=0; i<toDelete.size(); i++) {
                    debug_str(Fmt("clean dead cp, %s",toDelete[i]->AsString()).c_str());
                    m_dead_cproxy_clients.erase(toDelete[i]);
                }
            }
            for(size_t i=0; i<toDelete.size(); i++) {
                delete toDelete[i];
            }
        }

        //clean wb
        {
            vector<WbClient*> v;
            vector<Client*> vSkt;
            {
                AutoMutex auto1(m_wb_list_lock);
                SYSTEMTIME timeNow;
                GetLocalTime(&timeNow);
                for (map<string, WbClient*>::iterator it = m_wb_list.begin(); it != m_wb_list.end(); it++) {
                    if (SecondsBetween(it->second->m_last_time,timeNow) > 60) {
                        v.push_back(it->second);
                        vSkt.push_back(it->second->m_tcp_client);
                    }
                }
                for (vector<WbClient*>::iterator it = v.begin(); it != v.end(); it++) {
                    m_wb_list.erase((*it)->m_id);
                    (*it)->m_tcp_client->Close();
                }
            }
            for (vector<Client*>::iterator it = vSkt.begin(); it != vSkt.end(); it++) {
                if (*it) {
                    (*it)->Close();
                }
            }
        }
        //clean cp
        {
            vector<CProxyClient*> v;
            vector<Client*> vSkt;
            {
                AutoMutex auto1(m_cp_list_lock);
                SYSTEMTIME timeNow;
                GetLocalTime(&timeNow);
                for (map<string, CProxyClient*>::iterator it = m_cp_list.begin(); it != m_cp_list.end(); it++) {
                    if (SecondsBetween(it->second->m_last_time,timeNow) > 60) {
                        v.push_back(it->second);
                        vSkt.push_back(it->second->m_tcp_client);
                    }
                }
                for (vector<CProxyClient*>::iterator it = v.begin(); it != v.end(); it++) {
                    m_cp_list.erase((*it)->m_id);
                    (*it)->m_tcp_client->Close();
                }
            }
            for (vector<Client*>::iterator it = vSkt.begin(); it != vSkt.end(); it++) {
                if (*it) {
                    (*it)->Close();
                }
            }
        }
    }
}
void AServer::FreeProxyService(string mac) {
    INFO(Fmt("AServer::FreeProxyService(%s)", mac.c_str()).c_str());
    AutoMutex auto1(m_proxys_lock);
    map<string,ProxyServer*>::iterator it = m_proxys.find(mac);
    if (it == m_proxys.end()) {
        return;
    }
    m_free_proxys.push_back(it->second);
    it->second->m_mac.clear();
    m_proxys.erase(it);
}
string AServer::GetCProxyList() {
    INFO("AServer::GetCProxyList()");
    AutoMutex auto1(m_cproxy_clients_lock);
    string ret;
    for (set<CProxyClient*>::iterator it = m_cproxy_clients.begin(); it != m_cproxy_clients.end(); it++) {
        if (ret.empty()) {
            ret = (*it)->m_mac;
        } else {
            ret = ret + "&" + (*it)->m_mac;
        }
    }
    ret += "\n";
    INFO(Fmt("AServer::GetCProxyList(): %s",ret.c_str()).c_str());
    return ret;
}
ProxyServer* AServer::CreateProxyService(CProxyClient* cp) {
    INFO(Fmt("AServer::CreateProxyService(%s)",cp->AsString()).c_str());
    if (!cp)
        return NULL;
    if (cp->m_type != cctControl || cp->m_state != ccsMacRecved) {
        return NULL;
    }
    ProxyServer *ret = new ProxyServer;
    ret->SetLogger(m_log);
    if (!ret->StartListen()) {
        delete ret;
        return NULL;
    }
    ret->m_mac = cp->m_mac;
    AutoMutex auto1(m_proxys_lock);
    m_proxys[ret->m_mac] = ret;
    INFO(Fmt("AServer::CreateProxyService, success, port %s",ret->m_port.c_str()).c_str());
    return ret;
}
void AServer::RemoveProxyService(ProxyServer *ps) {
    INFO(Fmt("AServer::RemoveProxyService(%s)",ps->m_mac.c_str()).c_str());
    if (!ps)
        return;
    AutoMutex auto1(m_proxys_lock);
    for (map<string, ProxyServer*>::iterator it = m_proxys.begin(); it != m_proxys.end(); it++) {
        if (it->second == ps) {
            ps->Stop();
            m_proxys.erase(it);
            return;
        }
    }
}
void AServer::RemoveProxyService(string mac) {
    INFO(Fmt("AServer::RemoveProxyService(%s)",mac.c_str()).c_str());
    AutoMutex auto1(m_proxys_lock);
    map<string, ProxyServer*>::iterator it = m_proxys.find(mac);
    if (it != m_proxys.end()) {
        it->second->Stop();
        m_proxys.erase(it);
        return;
    }
}
string AServer::GetProxyList() {
    INFO("AServer::GetProxyList()");
    AutoMutex auto1(m_proxys_lock);
    string ret;
    for (map<string, ProxyServer*>::iterator it = m_proxys.begin(); it != m_proxys.end(); it++) {
        if (!ret.empty()) {
            ret += "&";
        }
        ret = ret + it->first + ":" + it->second->m_port;
    }
    ret += "\n";
    INFO(Fmt("AServer::GetProxyList(): %s", ret.c_str()).c_str());
    return ret;
}

string AServer::GetProxyService(string mac) {
    debug_str(Fmt("AServer::GetProxyService(%s)",mac.c_str()).c_str());
    {
        AutoMutex auto1(m_proxys_lock);
        map<string, ProxyServer*>::iterator it = m_proxys.find(mac);
        if (it != m_proxys.end()) {
            debug_str(Fmt("find proxy service, port %s",it->second->m_port.c_str()).c_str());
            return it->second->m_port;
        }
        if (!m_free_proxys.empty()) {
            ProxyServer* ret = m_free_proxys.back();
            ret->m_mac = mac;
            m_proxys[mac] = ret;
            m_free_proxys.pop_back();
            return ret->m_port;
        }
        if (m_proxys.size() >= (size_t)m_max_proxy) {
            ERR(Fmt("proxys is more than %d,get proxy fail.",m_max_proxy).c_str());
            return "\n";
        }
    }
    AutoMutex auto1(m_cproxy_clients_lock);
    for (set<CProxyClient*>::iterator it = m_cproxy_clients.begin(); it != m_cproxy_clients.end(); it++) {
        if ((*it)->m_mac == mac && (*it)->m_state == ccsMacRecved) {
            ProxyServer *ps = CreateProxyService(*it);
            if (NULL == ps) {
                ERR("create proxy service fail");
                return "\n";
            } else {
                INFO("create proxy service success");
                return ps->m_port + "\n";
            }
        }
    }
    return "\n";
}
void AServer::AddCProxyClient(CProxyClient *clt) {
    debug_str(Fmt("AServer::AddCProxyClient(%s)",clt->AsString()).c_str());
    if (!clt)
        return;
    AutoMutex auto1(m_cproxy_clients_lock);
    m_cproxy_clients.insert(clt);
}
void AServer::RemoveCProxyClient(CProxyClient *clt) {
    debug_str(Fmt("AServer::RemoveCProxyClient(%s)",clt->AsString()).c_str());
    if (!clt)
        return;
    AutoMutex auto1(m_cproxy_clients_lock);
    set<CProxyClient*>::iterator it = m_cproxy_clients.find(clt);
    if (it == m_cproxy_clients.end()) {
        debug_str("not found cp");
        return;
    }
    clt->m_tcp_client->SetData(NULL);
    clt->m_tcp_client->Close();
    SYSTEMTIME st;
    GetLocalTime(&st);
    m_dead_cproxy_clients[clt] = st;
    m_cproxy_clients.erase(it);
    if (!FindCProxy(clt->m_mac)) {
        FreeProxyService(clt->m_mac);
    }
}
CProxyClient* AServer::FindCProxy(string mac) {
    debug_str(Fmt("AServer::FindCProxy(%s)", mac.c_str()).c_str());
    AutoMutex auto1(m_cproxy_clients_lock);
    for (set<CProxyClient*>::iterator it = m_cproxy_clients.begin(); it != m_cproxy_clients.end(); it++) {
        if ((*it)->m_mac == mac) {
            return *it;
        }
    }
    return NULL;
}

const char * const _aserver_cfg = "[SERVER]\n\
heartbeat=1\n\
heartbeat_interval_second=30\n\
linger=0\n\
listenqueue_length=50\n\
sendbuf_size=-1\n\
recvbuf_szie=-1\n\
bind_ip=0.0.0.0\n\
listen_port=8888\n\
log_path=\n\
worker_count=5\n\
consumer_count=3";

const char * const _aserver_as_cfg = "[SERVER]\n\
heartbeat=1\n\
heartbeat_interval_second=30\n\
linger=0\n\
listenqueue_length=50\n\
sendbuf_size=-1\n\
recvbuf_szie=-1\n\
bind_ip=0.0.0.0\n\
listen_port=8889\n\
log_path=\n\
worker_count=1\n\
consumer_count=1";
bool AServer::StartListen() {
    INFO("AServer::StartListen()");
    string scfg(LoadStringFromFile(ExtractFilePath(ParamStr(0),true)+"server.ini"));
    if (scfg.empty()) {
        scfg.assign(_aserver_cfg);
    }
    INFO(Fmt("server config:\n%s",scfg.c_str()).c_str());
    m_tcp_server = new TcpServer(scfg, m_log);
    if (!m_tcp_server) {
        ERR("AServer::StartListen(), start tcp server fail");
        return false;
    }
    m_tcp_server->m_on_client_connected = OnClientConnected_cproxy;
    m_tcp_server->m_on_client_disconnected = OnClientDisconnected_cproxy;
    m_tcp_server->Startup();

    string ascfg(LoadStringFromFile(ExtractFilePath(ParamStr(0),true)+"as.ini"));
    if (ascfg.empty()) {
        ascfg.assign(_aserver_as_cfg);
    }
    INFO(Fmt("as config:\n%s",ascfg.c_str()).c_str());
    m_tcp_server_as = new TcpServer(ascfg, m_log);
    if (!m_tcp_server_as) {
        ERR("AServer::StartListen(), start as server fail");
        return false;
    }
    m_tcp_server_as->m_on_client_connected = OnClientConnected_as;
    m_tcp_server_as->m_on_client_disconnected = OnClientDisconnected_as;
    m_tcp_server_as->Startup();
    return true;
}

UUIDInfo* AServer::FindUUID(string uuid) {
    AutoMutex auto1(m_uuid_list_lock);
    map<string, UUIDInfo*>::iterator it = m_uuid_list.find(uuid);
    if (it == m_uuid_list.end()) {
        ERR(Fmt("find uuid %s fail", uuid.c_str()).c_str());
        return NULL;
    } else {
        debug_str(Fmt("find uuid %s success", uuid.c_str()).c_str());
        return it->second;
    }
}
void AServer::AddUUID(string uuid, UUIDInfo* cp) {
    debug_str(Fmt("AddUUID, %s", uuid.c_str()).c_str());
    AutoMutex auto1(m_uuid_list_lock);
    m_uuid_list[uuid] = cp;
}
void AServer::RemoveUUID(string uuid) {
    debug_str(Fmt("RemoveUUID, %s",uuid.c_str()).c_str());
    AutoMutex auto1(m_uuid_list_lock);
    map<string, UUIDInfo*>::iterator it = m_uuid_list.find(uuid);
    if (it != m_uuid_list.end()) {
        delete it->second;
        m_uuid_list.erase(it);
    } else {
        ERR("remove uuid fail, not found.");
    }
}
void AServer::AddWb(WbClient* wb) {
    if (wb == NULL)
        return;
    AutoMutex auto1(m_wb_list_lock);
    m_wb_list[wb->m_id] = wb;
}
void AServer::AddCp(CProxyClient* cp) {
    if (cp == NULL)
        return;
    AutoMutex auto1(m_cp_list_lock);
    m_cp_list[cp->m_id] = cp;
}
void AServer::RemoveWb(string id) {
    Client *skt = NULL;
    {
        AutoMutex auto1(m_wb_list_lock);
        map<string, WbClient*>::iterator it = m_wb_list.find(id);
        if (it != m_wb_list.end()) {
            skt = it->second->m_tcp_client;
            m_wb_list.erase(it);
            _aserver->m_wb_pool->Push(it->second);
        }
    }
    if (skt) {
        skt->Close();
    }
}

void AServer::RemoveCp(string id) {
    Client* skt = NULL;
    {
        AutoMutex auto1(m_cp_list_lock);
        map<string, CProxyClient*>::iterator it = m_cp_list.find(id);
        if (it != m_cp_list.end()) {
            skt = it->second->m_tcp_client;
            m_cp_list.erase(it);
            _aserver->m_cp_pool->Push(it->second);
        }
    }
    if (skt)
        skt->Close();
}

void AServer::CreateTransferClient(WbClient *wb, PooledHttpRequest *req, string hostData, string mac) {
    debug_str(Fmt("AServer::CreateTransferClient(%s)", mac.c_str()).c_str());
    CProxyClient* clt = FindCProxy(mac);
    if (!clt) {
        debug_str("find proxy server fail.");
        return ;
    }
    string uuid(CreateGUID());
    debug_str(Fmt("New uuid: %s", uuid.c_str()).c_str());
    UUIDInfo *info = new UUIDInfo();
    info->wb = wb;
    info->wbRequest = req;
    info->hostData = hostData;
    AddUUID(uuid, info);
    string sSend("t:");
    sSend += uuid;
    sSend += "\n";
    void *bufSend = MyMalloc(sSend.length());
    memcpy(bufSend,sSend.c_str(),sSend.length());
    clt->m_tcp_client->Send(bufSend,sSend.length(),0);
}

void OnClientConnected_cproxy(Client *client) {
    debug_str("OnClientConnected_cproxy");
    client->m_on_recved = OnRecved_cproxy;
    client->m_on_sent = OnSent_cproxy;
    CProxyClient *cp = (CProxyClient*)(_aserver->m_cp_pool->Pull());
    if (NULL == cp)
        cp = new CProxyClient();
    GetLocalTime(&cp->m_last_time);
    debug_str(cp->AsString());
    cp->m_tcp_client = client;
    client->SetData(cp);
    AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
    SYSTEMTIME st;
    GetLocalTime(&st);
    _aserver->m_born_cproxy_clients[cp] = st;
}
void OnClientDisconnected_cproxy(Client *client) {
    debug_str("OnClientDisconnected_cproxy");
    if (client->m_data == NULL) {
        debug_str("OnClientDisconnected_cproxy, client->m_data is NULL");
        return;
    }
    CProxyClient *clt = (CProxyClient*)(client->m_data);
    client->SetData(NULL);
    if (clt->m_type == cctControl) {
        debug_str("OnClientDisconnected_cproxy, remove c cp");
        _aserver->RemoveCProxyClient(clt);
    } else if (clt->m_type == cctTransfer) {
        debug_str("OnClientDisconnected_cproxy, push t cp");
        _aserver->RemoveCp(clt->m_id);
    }
}
void OnRecved_cproxy(Client *c,void *buf,size_t len) {
    debug_str("OnRecved_cproxy");
    if (!c || !buf || len == 0) {
        ERR("if (!c || !buf || len == 0)");
        return;
    }
    if (c->m_data == NULL) {
        ERR("if (c->m_data == NULL) {");
        c->Close();
        return;
    }
    CProxyClient *cp = (CProxyClient*)(c->m_data);
    GetLocalTime(&cp->m_last_time);
    if (cp->m_type == cctUnknown) {
        debug_str(Fmt("cctUnknown,%s",cp->AsString()).c_str());
        size_t minLen = sizeof(cp->m_packet_buf) - cp->m_packet_buf_off;
        if (minLen > len) {
            minLen = len;
        }
        if (minLen < len) {
            ERR("if (minLen < len) {");
            AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
            c->SetData(NULL);
            _aserver->m_born_cproxy_clients.erase(cp);
            delete cp;
        }
        memcpy(cp->m_packet_buf+cp->m_packet_buf_off,buf,minLen);
        cp->m_packet_buf_off += minLen;
        for (size_t i = cp->m_packet_buf_off-1; i > 0; i--) {
            if (cp->m_packet_buf[i] == '\n') {
                cp->m_packet_buf_off = 0;
                string s(cp->m_packet_buf,i);
                debug_str(string((char*)buf,len).c_str());
                debug_str(s.c_str());
                string l,r;
                SplitString(s,":",l,r);
                if (l=="c") {
                    debug_str("is control cp");
                    cp->m_type = cctControl;
                    cp->m_state = ccsMacRecved;
                    cp->m_mac = r;
                    {
                        AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
                        _aserver->m_born_cproxy_clients.erase(cp);
                    }
                    _aserver->AddCProxyClient(cp);
                } else if (l=="t"){
                    debug_str("is transfer cp");
                    cp->m_type = cctTransfer;
                    UUIDInfo *uuidInfo = _aserver->FindUUID(r);
                    if (!uuidInfo) {
                        ERR(Fmt("find uuid %s,fail",r.c_str()).c_str());
                        cp->m_tcp_client->Close();

                        {
                            AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
                            c->SetData(NULL);
                            _aserver->m_born_cproxy_clients.erase(cp);
                            delete cp;
                        }
                    } else {
                        _aserver->AddCp(cp);
                        debug_str("find uuid ok,move cp from bornlist to cplist");
                        {
                            AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
                            _aserver->m_born_cproxy_clients.erase(cp);
                        }
                        uuidInfo->cpTransfer = cp;
                        cp->m_wb_client = uuidInfo->wb;
                        cp->m_wb_uuid = r;
                        if (wctTunnel == uuidInfo->wb->m_type)
                            uuidInfo->wb->m_tunnel_client = cp;
                        else
                            uuidInfo->wb->m_tunnel_client = NULL;
                        if (uuidInfo->wbRequest) {
                            cp->m_http_request = uuidInfo->wbRequest;
                            uuidInfo->wbRequest->m_cp_client = cp;

                        }
                        cp->m_state = ccsUUIDRecved;
                        debug_str(Fmt("send host-data to cp: %s", uuidInfo->hostData.c_str()).c_str());
                        void *bufSend = MyMalloc(uuidInfo->hostData.length());
                        memcpy(bufSend,uuidInfo->hostData.c_str(),uuidInfo->hostData.length());
                        cp->m_tcp_client->Send(bufSend,uuidInfo->hostData.length(),0);
                        _aserver->RemoveUUID(r);
                    }
                } else {
                    ERR("not c & t,unexcept data.");
                    cp->m_tcp_client->Close();
                    {
                        AutoMutex auto1(_aserver->m_born_cproxy_clients_lock);
                        c->SetData(NULL);
                        _aserver->m_born_cproxy_clients.erase(cp);
                        delete cp;
                    }
                }
                break;
            }
        }
    } else if (cp->m_type == cctControl) {
        ERR("control client should not send data to server");
        _aserver->RemoveCProxyClient(cp);
    } else if (cp->m_type == cctTransfer) {
        debug_str(Fmt("(cp->m_type == cctTransfer),wb_uuid: %s",cp->m_wb_uuid.c_str()).c_str());
        if (ccsUUIDRecved == cp->m_state) {
            if (!cp->m_wb_client) {
                _aserver->RemoveCp(cp->m_id);
                //_aserver->m_cp_pool->Push(cp);
            }
            if ((wctTunnel == cp->m_wb_client->m_type  && cp->m_wb_client->m_state != wcsTunnelCreated)
                    || (cp->m_wb_client && cp->m_http_request && wctHttp == cp->m_wb_client->m_type && !cp->m_http_request->m_remote_connected)){
                if (len<3) {
                    ERR("cp client == uuidrecved , wb client not remoteconnected,datalen < 3, unexcepted.");
                    _aserver->RemoveCp(cp->m_id);
                    //_aserver->m_cp_pool->Push(cp);
                } else {
                    debug_str("this is remoteconnected signal from cp client.")
                    debug_str("uuid")
                    char *sbuf = (char*)buf;
                    if (sbuf[2] == '\n') { //ok\n
                        if (wctTunnel == cp->m_wb_client->m_type) {
                            debug_str("send tunnelOK");
                            string tunnelOK("HTTP/1.1 200 Connection Established\r\n\r\n");
                            void *bufSend = MyMalloc(tunnelOK.length());
                            memcpy(bufSend,tunnelOK.c_str(),tunnelOK.length());
                            cp->m_wb_client->m_tcp_client->Send(bufSend,tunnelOK.length(),0);
                            cp->m_wb_client->m_tunnel_client = cp;
                            cp->m_wb_client->m_state = wcsTunnelCreated;
                            cp->m_wb_client->SendCache();
                        } else {
                            cp->m_http_request->m_remote_connected = true;
                            cp->m_http_request->SendCache();
                        }
                        int bufRetain = len - 3;
                        if (bufRetain > 0) {
                            debug_str("bufRetain > 0");
                            void *bufSend = MyMalloc(bufRetain);
                            memcpy(bufSend,(char*)buf + 3, bufRetain);
                            cp->m_wb_client->m_tcp_client->Send(bufSend,bufRetain,0);
                        }
                    } else {
                        _aserver->RemoveCp(cp->m_id);
                        //_aserver->m_cp_pool->Push(cp);
                    }
                }
            } else {
                debug_str("mapped ok ,transfer data");
                void *bufSend = MyMalloc(len);
                memcpy(bufSend,buf,len);
                cp->m_wb_client->m_tcp_client->Send(bufSend,len,0);
            }
        } else {
            _aserver->RemoveCp(cp->m_id);
            //_aserver->m_cp_pool->Push(cp);
        }
    } else {
        ERR("unexcepted cp state");
        c->Close();
        return;
    }

}
void OnSent_cproxy(Client *c,void *buf,size_t len,bool success ) {
    debug_str("OnSent_cproxy");
    if (buf) {
        MyFree(buf);
    }
    if (c && c->m_data) {
        GetLocalTime(&((CProxyClient*)(c->m_data))->m_last_time);
    }
}

void OnClientConnected_wb(Client *client) {
    debug_str("OnClientConnected_wb");
    client->m_on_recved = OnRecved_wb;
    client->m_on_sent = OnSent_wb;
    if (!client) {
        return;
    }
    WbClient *clt = (WbClient*)(_aserver->m_wb_pool->Pull());
    if (NULL == clt)
        clt = new WbClient();
    GetLocalTime(&clt->m_last_time);
    string sport = FormatString("%d",client->m_listen_port);
    {
        AutoMutex auto1(_aserver->m_proxys_lock);
        for (map<string, ProxyServer*>::iterator it = _aserver->m_proxys.begin(); it != _aserver->m_proxys.end(); it++) {
            if (it->second->m_port == sport) {
                debug_str("wb\'s proxy server found.");
                clt->m_proxy_server = it->second;
                break;
            }
        }
    }
    if (NULL == clt->m_proxy_server) {
        delete clt;
        client->SetData(NULL);
        client->Close();
    }
    clt->m_tcp_client = client;
    client->SetData(clt);
    clt->m_state = wcsConnected;
    _aserver->AddWb(clt);

}
void OnClientDisconnected_wb(Client *client) {
    debug_str("OnClientDisconnected_wb");
    if (!client) {
        debug_str("OnClientDisconnected_wb, client is NULL");
        return;
    }
    if (NULL == client->m_data) {
        debug_str("OnClientDisconnected_wb, m_data is NULL");
        return;
    }
    WbClient *clt = (WbClient*)(client->m_data);
    client->SetData(NULL);
    if (clt->m_tunnel_client && clt->m_tunnel_client->m_tcp_client) {
        debug_str("OnClientDisconnected_wb, close cp");
        clt->m_tunnel_client->m_tcp_client->Close();
    }
    _aserver->RemoveWb(clt->m_id);
}
void OnRecved_wb(Client *c,void *buf,size_t len) {
    debug_str("OnRecved_wb");
    if (!c || !buf || len == 0) {
        return;
    }

    if (NULL == c->m_data) {
        c->Close();
        return;
    }

    WbClient *clt = (WbClient*)(c->m_data);
    GetLocalTime(&clt->m_last_time);
    if (wcsTunnelCreated == clt->m_state) {
        debug_str("if (wcsTunnelCreated == clt->m_state) {");
        void *bufSend = MyMalloc(len);
        memcpy(bufSend,buf,len);
        clt->PushBuf(bufSend,len);
        clt->SendCache();
        return;
    }
    if (clt->m_state != wcsConnected) {
        c->Close();
        return;
    }
    if (clt->m_state == wcsConnected) {
        debug_str("if (clt->m_state == wcsConnected) {");
        int minLen = sizeof(clt->m_packet_buf) - clt->m_packet_buf_off;
        if (minLen < (int)len) {
            ERR("error data");
            c->SetData(NULL);
            _aserver->RemoveWb(clt->m_id);
            return;
        } else {
            minLen = len;
        }
        memcpy(clt->m_packet_buf + clt->m_packet_buf_off, buf, minLen);
        clt->m_packet_buf_off += minLen;
        string header;
        char *iBodyStart = NULL;
        bool bHeaderDone = false;
        for (int i = 0; i <= clt->m_packet_buf_off - 4;  i++) {
            if (strncmp("\r\n\r\n", clt->m_packet_buf+i, 4) == 0) {
                bHeaderDone = true;
                iBodyStart = clt->m_packet_buf + i + 4;
                header.assign(clt->m_packet_buf,i+4);
                debug_str(Fmt("wb is httpcompleted, http request: %s",header.c_str()).c_str());
                if (header.substr(header.length()-4,4) != "\r\n\r\n") {
                    debug_str("fuck,fuck");
                }
                break;
            }
        }
        if (!bHeaderDone) {
            debug_str("wb is http header uncomplete.");
            return;
        }
        bool sameHost = false;
        bool bGet = strncmp("GET",clt->m_packet_buf,3) == 0;
        bool bPost = strncmp("POST",clt->m_packet_buf,4) == 0;
        if (bGet || bPost) {
            debug_str("Get or Post");
            clt->m_type = wctHttp;
            PooledHttpRequest *req = (PooledHttpRequest*)(_aserver->m_http_request_pool->Pull());
            if (NULL == req)
                req = new(PooledHttpRequest);
            req->m_wb = clt;
            char *idx = strstr(clt->m_packet_buf, "\r\n");
            if (!idx) {
                ERR("get or post,not find \r\n");
                c->SetData(NULL);
                _aserver->RemoveWb(clt->m_id);
                //_aserver->m_wb_pool->Push(clt);
                _aserver->m_http_request_pool->Push(req);
                return;
            }

            req->m_content_length = 0;
            if (bPost) {
                const char *clStart = strcasestr(header.c_str(),"Content-Length");
                if (clStart) {
                    const char *clEnd = strstr(clStart,"\r\n");
                    if (clEnd) {
                        const char *clStartX = strstr(clStart,":");
                        if (clStartX && clStartX + 1 < clEnd) {
                               req->m_content_length = atoi(clStartX+1);
                        }
                    }
                }
            }
            if (req->m_content_length > 0) {
                if (iBodyStart + req->m_content_length <= clt->m_packet_buf + clt->m_packet_buf_off) {
                    req->m_body = string(iBodyStart,req->m_content_length);
                    req->m_data_completed = true;
                }
            } else {
                req->m_body.clear();
                req->m_data_completed = true;
            }

            if (!req->m_data_completed) {
                debug_str("request not completed.");
                _aserver->m_http_request_pool->Push(req);
                return;
            }

            char *hostBegin = NULL;
            char *hostEnd = NULL;
            for (char *i = clt->m_packet_buf; i < idx; i++) {
                if (*i == ' ') {
                    if (!hostBegin) {
                        hostBegin = i+1;
                    } else if (!hostEnd) {
                        hostEnd = i;
                        break;
                    }
                }
            }
            if (!hostBegin || !hostEnd || hostEnd - hostBegin < 3) {
                ERR("get or post,protocol invalid.");
                c->SetData(NULL);
                _aserver->RemoveWb(clt->m_id);
                //_aserver->m_wb_pool->Push(clt);
                _aserver->m_http_request_pool->Push(req);
                return;
            }

            string urlOri(hostBegin,hostEnd - hostBegin);
            debug_str(urlOri.c_str());
            string url(urlOri);
            url = ReplaceStringI(url,"http://","",false);
            debug_str(url.c_str());
            for(size_t i = 0; i<url.length();i++) {
                if (url[i] == '/') {
                    string tmpHost = url.substr(0,i);
                    sameHost = (tmpHost == req->m_host);
                    if (!sameHost)
                        req->m_host = url.substr(0,i);
                    req->m_path = url.substr(i);
                    string tmpHeader(header);
                    req->m_http_proxy_request = header;
                    ReplaceString(req->m_http_proxy_request,urlOri,req->m_path, false);
                    debug_str(Fmt("host: %s\npath: %s\nnew request: %s",req->m_host.c_str(),req->m_path.c_str(),req->m_http_proxy_request.c_str()).c_str());
                    if (req->m_http_proxy_request[req->m_http_proxy_request.length()-1] != '\n') {
                        debug_str("err request");
                    }
                    break;
                }
            }

            if (req->m_host.empty() || req->m_http_proxy_request.empty()) {
                ERR("error wb request data");
                c->SetData(NULL);
                _aserver->RemoveWb(clt->m_id);
                //_aserver->m_wb_pool->Push(clt);
                _aserver->m_http_request_pool->Push(req);
                return;
            }

            clt->m_current_request = req;
            clt->m_requests.push_back(req);

        } else if (strncmp("CONNECT",clt->m_packet_buf,7) == 0) {
            debug_str("tunnel mode");
            clt->m_type = wctTunnel;
            char *headEnd = strstr(clt->m_packet_buf, "\r\n\r\n");
            char *idx = strstr(clt->m_packet_buf, "\r\n");
            if (!headEnd) {
                ERR("invalid tunnel protocol");
                c->SetData(NULL);
                _aserver->RemoveWb(clt->m_id);
                return;
            }

            int retainLen = clt->m_packet_buf + clt->m_packet_buf_off - headEnd - 4;
            int retainBufLen = len - minLen;

            if (retainLen > 0 || retainBufLen > 0) {
                void *bufNew = MyMalloc(retainLen + retainBufLen);
                if (retainLen > 0) {
                    memcpy(bufNew,headEnd + 4,retainLen);
                }
                if (retainBufLen > 0) {
                    memcpy(bufNew,AddPtr(buf,minLen),retainBufLen);
                }
                clt->PushBuf(bufNew,retainBufLen+retainLen);
            }

            char *hostBegin = NULL;
            char *hostEnd = NULL;
            for (char *i = clt->m_packet_buf; i < idx; i++) {
                if (*i == ' ') {
                    if (!hostBegin) {
                        hostBegin = i+1;
                    } else if (!hostEnd) {
                        hostEnd = i;
                        break;
                    }
                }
            }
            if (!hostBegin || !hostEnd || hostEnd - hostBegin < 3) {
                ERR("parse tunnel field fail.");
                c->SetData(NULL);
                _aserver->RemoveWb(clt->m_id);
                //_aserver->m_wb_pool->Push(clt);
                return;
            }
            clt->m_host.assign(hostBegin,hostEnd - hostBegin);
            debug_str(Fmt("tunnel host: %s",clt->m_host.c_str()).c_str())
        } else {
            ERR("invalid wb request");
            c->SetData(NULL);
            _aserver->RemoveWb(clt->m_id);
            //_aserver->m_wb_pool->Push(clt);
            return;
        }

        string s;
        if (wctTunnel == clt->m_type) {
            debug_str("wctTunnel");
            s = s + "https:" + clt->m_host + "\n";
        } else if (wctHttp == clt->m_type) {
            debug_str("wctHttp");
            s = s + "http:" + clt->m_current_request->m_host + "\n";
        } else {
            debug_str("invalid wbclient state");
            c->SetData(NULL);
            _aserver->RemoveWb(clt->m_id);
            //_aserver->m_wb_pool->Push(clt);
            return;
        }

        if (wctTunnel == clt->m_type || !sameHost) {
            _aserver->CreateTransferClient(clt, clt->m_current_request, s, clt->m_proxy_server->m_mac);
        }

        if (clt->m_type == wctHttp && !clt->m_current_request->m_http_proxy_request.empty()) {
            void *bufCache = MyMalloc(clt->m_current_request->m_http_proxy_request.length());
            memcpy(bufCache, clt->m_current_request->m_http_proxy_request.c_str(), clt->m_current_request->m_http_proxy_request.length());
            clt->m_current_request->PushBuf(bufCache,clt->m_current_request->m_http_proxy_request.length());
            debug_str(Fmt("buf of http typed request: %s",clt->m_current_request->m_http_proxy_request.c_str()).c_str());
            //send body
            if (clt->m_current_request->m_content_length > 0) {
                void *bufBody = MyMalloc(clt->m_current_request->m_content_length);
                memcpy(bufBody,clt->m_current_request->m_body.c_str(),clt->m_current_request->m_content_length);
                clt->m_current_request->PushBuf(bufBody,clt->m_current_request->m_content_length);
                debug_str(Fmt("request body len: %d\n",clt->m_current_request->m_content_length).c_str());
            }
            if (sameHost) {
                clt->m_current_request->SendCache();
            }
        }

        clt->m_packet_buf_off = 0;
    }
}
void PooledHttpRequest::SendCache() {
    while (!m_cached_bufs.empty()) {
        BufBlock buf = m_cached_bufs.front();
        m_cached_bufs.pop_front();
        debug_str(Fmt("send wb request cache,len: %d",buf.len).c_str());
        if (m_cp_client && m_cp_client->m_tcp_client) {
            m_cp_client->m_tcp_client->Send(buf.buf,buf.len,0);
        } else {
            return;
        }
    }
}
const char* WbClient::AsString(){
    return m_id.c_str();
}
void WbClient::Fina() {
    debug_str("WbClient::Fina()");
    if (m_tcp_client) {
        m_tcp_client->Close();
    }
    debug_str(Fmt("wb requests: %d", m_requests.size()).c_str());
    for (size_t i = 0 ; i < m_requests.size(); i++) {
        debug_str("push request");
        _aserver->m_http_request_pool->Push(m_requests[i]);
    }
    m_requests.clear();
}

void WbClient::PushBuf(void *buf, size_t len) {
    debug_str(Fmt("push wb buf,len %d",len).c_str());
    m_cached_bufs.push_back(BufBlock(buf,len));
}
void WbClient::SendCache() {
    while (!m_cached_bufs.empty()) {
        BufBlock buf = m_cached_bufs.front();
        m_cached_bufs.pop_front();
        debug_str(Fmt("send wb cache,len: %d",buf.len).c_str());
        if (m_tunnel_client && m_tunnel_client->m_tcp_client) {
            m_tunnel_client->m_tcp_client->Send(buf.buf,buf.len,0);
        } else {
            return;
        }
    }
}

void CProxyClient::Fina() {
    debug_str("CProxyClient::Fina()");
    if (m_tcp_client) {
        debug_str("CProxyClient::Fina(), close tcp");
        m_tcp_client->Close();
    }
    if (m_wb_client && m_wb_client->m_tcp_client) {
        debug_str("CProxyClient::Fina(), close wb");
        m_wb_client->m_tcp_client->Close();
    }
}

void PooledHttpRequest::PushBuf(void *buf, size_t len) {
    debug_str(Fmt("push wb request buf,len %d",len).c_str());
    m_cached_bufs.push_back(BufBlock(buf,len));
}

void PooledHttpRequest::Fina() {
    debug_str("HttpRequest::Fina()");
    if (m_cp_client && m_cp_client->m_tcp_client) {
        debug_str("HttpRequest::Fina(), close cp");
        m_cp_client->m_tcp_client->Close();
    }

    while (!m_cached_bufs.empty()) {
        BufBlock bb = m_cached_bufs.front();
        free(bb.buf);
        m_cached_bufs.pop_front();
    }

}

void OnSent_wb(Client *c,void *buf,size_t len,bool success ) {
    debug_str("OnSent_wb");
    if (buf)
        MyFree(buf);
    if (c && c->m_data) {
        GetLocalTime(&((CProxyClient*)(c->m_data))->m_last_time);
    }
}

void OnClientConnected_as(Client *client) {
    debug_str("OnClientConnected_as");
    client->m_on_recved = OnRecved_as;
    client->m_on_sent = OnSent_as;
    SYSTEMTIME st;
    GetLocalTime(&st);
    AutoMutex auto1(_aserver->m_as_clients_lock);
    _aserver->m_as_clients[client] = st;
}
void OnClientDisconnected_as(Client *client) {
    debug_str("OnClientDisconnected_as");
    AutoMutex auto1(_aserver->m_as_clients_lock);
    _aserver->m_as_clients.erase(client);
}
void OnRecved_as(Client *c,void *buf,size_t len) {
    debug_str("OnRecved_as");
    if (!c || !buf || len == 0) {
        return;
    }
    string s((char*)buf,len);
    if (s[s.length()-1] != '\n') {
        ERR("as recved, invalid data");
        AutoMutex auto1(_aserver->m_as_clients_lock);
        c->Close();
        _aserver->m_as_clients.erase(c);
        return;
    }
    string cmd(s.substr(0,s.length()-1));
    cmd = trim(cmd);
    debug_str(Fmt("Cmd: %s",cmd.c_str()).c_str());
    string ret;
    if (SameText(cmd,"mac_list")) {
        debug_str("mac_list");
        ret = _aserver->GetCProxyList();
    } else if (cmd == string("proxy_list")) {
        debug_str("proxy_list");
        ret = _aserver->GetProxyList();
    } else if (cmd.substr(0,strlen("get_proxy")) == "get_proxy") {
        debug_str("get_proxy");
        string l,r;
        SplitString(cmd,",",l,r);
        ret =_aserver->GetProxyService(r);
    }
    debug_str(Fmt("as response: %s", ret.c_str()).c_str());
    void *bufSend = MyMalloc(ret.length());
    memcpy(bufSend,ret.c_str(),ret.length());
    c->Send(bufSend,ret.length(),0);
}
void OnSent_as(Client *c,void *buf,size_t len,bool success ) {
    debug_str("OnSent_as");
    if (buf) {
        MyFree(buf);
    }
}



