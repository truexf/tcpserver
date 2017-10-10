/*
 * fyshttpconnection.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: root
 */

#include "http_types.h"
#include <algorithm>
#include "../fyslib/xsocketutils.hpp"
#include "../fyslib/AutoObject.hpp"
#include "../fyslib/xtimer.h"
using namespace fyslib;

FysHttpConnectionManager::FysHttpConnectionManager() {
    m_connections_lock = CreateMutex(true);
    m_picktype = Rotate;
}

FysHttpConnectionManager::~FysHttpConnectionManager() {
    DestroyMutex(m_connections_lock);
}

void FysHttpConnectionManager::Run() {
    timespec ts;
    ts.tv_sec = 300;
    ts.tv_nsec = 0;
    for (;;) {
        if (GetTerminated())
            break;
        SleepExactly(&ts);
        {
            AutoMutex a1(m_connections_lock);
            for (map<string, vector<string>* >::iterator it = m_dns.begin(); it != m_dns.end(); it++) {
                string l,r;
                SplitString(it->first.c_str(),":",l,r);
                LookupDomain(l.c_str(),r.c_str(),false);
            }
        }
    }
}

void FysHttpConnectionManager::LookupDomain(const char *domain, const char *port, bool clean) {
    AutoMutex a1(m_connections_lock);
    vector<string> ips;
    if (-1 == DnsLookup(domain, ips)) {
        return;
    }
    vector<string> *ipsPtr = new vector<string>;
    ipsPtr->reserve(ips.size());
    for(size_t i = 0; i<ips.size(); i++) {
        ipsPtr->push_back(ips[i]);
    }
    vector<string> invalidIps;
    string host(Fmt("%s:%s",domain,":",port));
    map<string, vector<string>* >::iterator it = m_dns.find(host);
    if (it != m_dns.end()) {
        for(vector<string>::iterator itOld = it->second->begin(); itOld != it->second->end(); itOld++) {
            if (std::find(ips.begin(),ips.end(), *itOld) == ips.end()) {
                invalidIps.push_back(*itOld);
            }
        }
        delete it->second;
        m_dns.erase(it);
    }
    m_dns[host] = ipsPtr;

    if (clean) {
        {
            vector<map<string, vector<FysHttpConnection*>*>*> invalidConnectionMap;
            vector<string> invalidDomain;
            for(map<string,map<string, vector<FysHttpConnection*>*>* >::iterator it = m_conections.begin(); it != m_conections.end(); it++) {
                if (m_dns.find(it->first) == m_dns.end()) {
                    for (map<string, vector<FysHttpConnection*>*>::iterator it0 = it->second->begin(); it0 != it->second->end(); it++) {
                        for(vector<FysHttpConnection*>::iterator it1=it0->second->begin();it1 != it0->second->end(); it1++) {
                            delete *it1;
                        }
                        delete it0->second;
                    }
                    invalidConnectionMap.push_back(it->second);
                    invalidDomain.push_back(it->first);
                }
            }
            for (size_t i = 0; i<invalidConnectionMap.size(); i++){
                delete invalidConnectionMap[i];
            }
            for (size_t i=0; i<invalidDomain.size(); i++) {
                m_conections.erase(invalidDomain[i]);
            }
        }
    }
}

FysHttpConnection* FysHttpConnectionManager::PullConnection(const char * domain, const char * port, const timeval *tv) {
    LockMutex(m_connections_lock);
    FysHttpConnection *ret = NULL;
    string host(Fmt("%s:%s",domain,":",port));
    map<string, vector<string>* >::iterator itIps = m_dns.find(host);
    if (itIps == m_dns.end()) {
        LookupDomain(domain,port,false);
    }
    itIps = m_dns.find(host);
    if (itIps == m_dns.end() || itIps->second->empty()) {
        UnlockMutex(m_connections_lock);
        return NULL;
    }

    PickType pkType = m_picktype;
    if (Rotate == pkType) {
        size_t idx = 0;
        map<string,size_t>::iterator it = m_ip_index.find(host);
        if (it == m_ip_index.end()) {
            m_ip_index[host] = idx;
        } else {
            idx = it->second + 1;
            if (idx >= itIps->second->size()) {
                idx = 0;
            }
            m_ip_index[domain] = idx;
        }
        string ip(itIps->second->at(idx));
        map<string, map<string, vector<FysHttpConnection*>*>* >::iterator itDomain = m_conections.find(host);
        map<string, vector<FysHttpConnection*>*> *connMap = NULL;
        if (itDomain == m_conections.end()) {
            connMap = new map<string, vector<FysHttpConnection*>*>;
        } else {
            connMap = itDomain->second;
        }
        vector<FysHttpConnection*> *conns = NULL;
        map<string, vector<FysHttpConnection*>*>::iterator itConn = connMap->find(ip);
        if (itConn == connMap->end()) {
            conns = new vector<FysHttpConnection*>;
        } else {
            conns = itConn->second;
        }

        if (conns->empty()) {
            UnlockMutex(m_connections_lock);
            ret = new FysHttpConnection;
            ret->SetHost(domain);
            ret->SetPort(port);
            ret->SetIp(itIps->second->at(0).c_str());
            if (!ret->Connect()) {
                delete ret;
                return NULL;
            } else {
                ret->m_mgr = this;
                return ret;
            }
        } else {
            ret = conns->back();
            conns->pop_back();
            UnlockMutex(m_connections_lock);
            return ret;
        }
    } else if (SpeedFirst == pkType) {
        map<string, map<string, vector<FysHttpConnection*>*>* >::iterator itDomain = m_conections.find(host);
        map<string, vector<FysHttpConnection*>*> *connMap = NULL;
        if (itDomain == m_conections.end()) {
            connMap = new map<string, vector<FysHttpConnection*>*>;
        } else {
            connMap = itDomain->second;
        }
        FysHttpConnection *connMostFast = NULL;
        map<string, vector<FysHttpConnection*>*>::iterator fastestIp = connMap->end();
        for(map<string, vector<FysHttpConnection*>*>::iterator itConn = connMap->begin(); itConn != connMap->end(); itConn++) {
            for (vector<FysHttpConnection*>::iterator connp = itConn->second->begin(); connp != itConn->second->end(); connp++) {
                if (NULL == connMostFast) {
                    connMostFast = *connp;
                    fastestIp = itConn;
                } else {
                    if (connMostFast->GetHeaderTime() < (*connp)->GetHeaderTime()) {
                        connMostFast = *connp;
                        fastestIp = itConn;
                    }
                }
            }
        }
        if (fastestIp != itDomain->second->end()) {
            ret = fastestIp->second->back();
            fastestIp->second->pop_back();
            UnlockMutex(m_connections_lock);
            return ret;
        } else {
            UnlockMutex(m_connections_lock);
            FysHttpConnection *conn = new FysHttpConnection;
            conn->SetHost(domain);
            conn->SetIp(itIps->second->at(0).c_str());
            conn->SetPort(port);
            conn->SetConnectTimeout(tv);
            if (!conn->Connect()) {
                delete conn;
                return NULL;
            } else {
                return ret;
            }
        }
    } else if (FirstIp == pkType){
        string ip;
        if (!itIps->second->empty()) {
            ip = *(itIps->second->begin());
        }
        map<string, map<string, vector<FysHttpConnection*>*>* >::iterator itDomain = m_conections.find(host);
        if (itDomain != m_conections.end()) {
            map<string, vector<FysHttpConnection*>*>::iterator itConn = itDomain->second->find(ip);
            if (itConn != itDomain->second->end()) {
                ret = itConn->second->back();
                itConn->second->pop_back();
            }
        }
        if (ret == NULL) {
            UnlockMutex(m_connections_lock);
            ret = new FysHttpConnection;
            ret->SetHost(domain);
            ret->SetIp(itIps->second->at(0).c_str());
            ret->SetPort(port);
            if (!ret->Connect()) {
                delete ret;
                return NULL;
            } else {
                return ret;
            }
        } else {
            UnlockMutex(m_connections_lock);
            return ret;
        }
    } else {
        UnlockMutex(m_connections_lock);
        return NULL;
    }

}

void FysHttpConnectionManager::PushConnection(FysHttpConnection *conn) {
    if (!conn)
        return;
    if (hcsConnected != conn->GetState() || conn->GetHost().empty() || conn->GetIp().empty() || conn->GetPort().empty()) {
        delete conn;
        return;
    }
    string domain(conn->GetHost()+":"+conn->GetPort());
    string ip(conn->GetIp());

    AutoMutex auto1(m_connections_lock);
    map<string, vector<string>*>::iterator itDns = m_dns.find(domain);
    if (itDns == m_dns.end()) {
        vector<string> *ips = new vector<string>;
        m_dns[domain] = ips;
        ips->push_back(ip);
    } else {
        bool found = false;
        for (vector<string>::iterator itIps = itDns->second->begin(); itIps != itDns->second->end(); itIps++) {
            if (*itIps == ip) {
                found = true;
            }
        }
        if (!found) {
            itDns->second->push_back(ip);
        }
    }

    map<string, vector<FysHttpConnection*>*> *conns = NULL;
    map<string, map<string, vector<FysHttpConnection*>*>* >::iterator itDomain = m_conections.find(domain);
    if (itDomain == m_conections.end()) {
        conns = new map<string, vector<FysHttpConnection*>*>;
        m_conections[domain] = conns;
    } else {
        conns = itDomain->second;
    }

    size_t connLimit = 100; //default
    map<string, size_t>::iterator itCap = m_pool_capacity.find(domain);
    if (itCap != m_pool_capacity.end()) {
        connLimit = itCap->second;
    }
    size_t connCount = 0;
    for (map<string, vector<FysHttpConnection*>*>::iterator it = conns->begin(); it != conns->end(); it++) {
        connCount += it->second->size();
        if (connCount >= connLimit) {
            delete conn;
            return;
        }
    }

    map<string, vector<FysHttpConnection*>*>::iterator itConns = conns->find(ip);
    vector<FysHttpConnection*> *vconn = NULL;
    if (itConns == conns->end()) {
        vconn = new vector<FysHttpConnection*>;
    } else {
        vconn = itConns->second;
    }
    if (std::find(vconn->begin(),vconn->end(),conn) == vconn->end()) {
        vconn->push_back(conn);
    }
}

FysHttpConnection::FysHttpConnection() {
    m_state = hcsUnconnected;
    m_tcpclient = NULL;
    m_connect_timeout.tv_sec = 10; //default
    m_connect_timeout.tv_usec = 0;
    m_header_timeout = 10000;
    m_sending_lock = CreateMutex(false);
    memset(&m_request_time, 0, sizeof(SYSTEMTIME));
    m_complete_chan = new Pandc<int>;
    m_response = NULL;
    m_request_stream = NULL;
    m_mgr = NULL;
}

FysHttpConnection::~FysHttpConnection() {
    if (m_tcpclient)
        GetTcpClientManager()->RemoveClient(m_tcpclient);
    if (m_sending_lock)
        DestroyMutex(m_sending_lock);
    if (m_complete_chan)
        delete m_complete_chan;
    if (m_request_stream)
        delete m_request_stream;
}

void _OnHttpClientDataRecved(TcpClient *c,void *buf, size_t len) {
    FysHttpConnection *conn = (FysHttpConnection*)(c->m_data);
    if (conn->m_state != hcsRequesting) {
        return;
    }
    conn->m_response->FromStream((char*)buf,len);
}
void _OnHttpClientDataSent(TcpClient *c,void *buf, size_t len) {
    FysHttpConnection *conn = (FysHttpConnection*)(c->m_data);
    delete conn->m_request_stream;
    conn->m_request_stream = NULL;
}

void _OnHttpClientError(TcpClient *c, int errCode) {
    FysHttpConnection *conn = (FysHttpConnection*)(c->m_data);
    delete conn->m_request_stream;
    conn->m_request_stream = NULL;
    delete conn;
}

void _OnHttpResponseState(void *data, const ProtoDataState state) {
    if (rsHeaderCompleted == state) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        FysHttpConnection *conn = (FysHttpConnection*)data;
        long long ms = MillisecondsBetween(conn->m_request_time,st);
        if (ms < 0) {
            conn->SetHeaderTime(0);
        } else {
            conn->SetHeaderTime((size_t)ms);
        }
    } else if (rsCompleted == state) {
        FysHttpConnection *conn = (FysHttpConnection*)data;
        conn->m_complete_chan->P(0); //signal 0:success, 1:error
    } else if (rsErrorProto == state) {
        FysHttpConnection *conn = (FysHttpConnection*)data;
        conn->m_complete_chan->P(1);
    }
}

bool FysHttpConnection::Connect() {
    if (m_tcpclient) {
        GetTcpClientManager()->RemoveClient(m_tcpclient);
        m_tcpclient = NULL;
    }
    m_tcpclient = GetTcpClientManager()->ConnectRemote(&m_connect_timeout,m_ip,(unsigned short)atoi(m_port.c_str()),
            _OnHttpClientDataRecved,_OnHttpClientDataSent,_OnHttpClientError,
            this);
    if (m_tcpclient) {
        m_state = hcsConnected;
    }
    return (m_tcpclient != NULL);
}

void FysHttpConnection::SetHeaderTime(size_t duration) {
    m_header_time.push_back(duration);
    while (m_header_time.size() > 100) {
        m_header_time.pop_front();
    }
}

size_t FysHttpConnection::GetHeaderTime() {
    size_t ret = 0;
    size_t cnt = 0;
    for (deque<size_t>::iterator it = m_header_time.begin(); it != m_header_time.end(); it++) {
        cnt++;
        ret += *it;
    }
    if (cnt == 0) {
        return 0;
    } else {
        return ret / cnt;
    }
}

int FysHttpConnection::SendRequest(FysHttpRequest *req, FysHttpResponse *res) {
    if (!req || !res) {
        return -1;
    }
    {
        AutoMutex a1(m_sending_lock);
        if (m_state != hcsConnected) {
            return -2;
        }
        m_state = hcsRequesting;
    }
    GetLocalTime(&m_request_time);
    int i;
    while (m_complete_chan->TimedC(NULL,i)) {
    }
    m_response = res;
    m_request_stream = req->ToStream();
    m_tcpclient->Send(m_request_stream->GetBuffer(),m_request_stream->GetSize());
    timespec ts;
    ts.tv_sec = m_header_timeout % 1000;
    ts.tv_nsec = (m_header_timeout - (ts.tv_sec * 1000)) * 1000000;
    bool waited = m_complete_chan->TimedC(&ts,i);
    if (waited) {
        return -1;
    }
    m_state = hcsConnected;
    m_mgr->PushConnection(this);

    return 0;
}



