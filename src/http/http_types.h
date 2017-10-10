/*
 * httpclient.h
 *
 *  Created on: Sep 5, 2017
 *      Author: root
 */

#ifndef HTTP_HTTP_TYPES_H_
#define HTTP_HTTP_TYPES_H_

#include "../tcpclient/tcpclient.h"
#include "../tcpserver/tcpserver.h"
#include "../fyslib/tthread.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/pandc.hpp"
#include <string>
#include <map>
#include <deque>
#include "../fyslib/systemtime.h"
using std::string;
using std::map;
using std::deque;
using namespace fyslib;

enum HttpChunkedDataState {
    hcdsUnknown,
    hcdsChunkHeaderUncompleted,
    hcdsChunkBodyUncompleted,
    hcdsChunkEnding,
    hcdsChunkCompleted
};

enum HttpMethod {
    hmGet,
    hmPost
};

enum HttpVersion {
    hv10,
    hv11  //default http1.1
};

enum ProtoDataState {
    rsErrorProto,
    rsHeaderUncompleted,
    rsHeaderCompleted,
    rsBodyUncompleted,
    rsCompleted
};


typedef void (*OnHttpProgress)(void *data, const ProtoDataState state);
class FysHttpRequest {
private:
    FysHttpRequest(const FysHttpRequest&);
    FysHttpRequest& operator =(const FysHttpRequest&);
protected:
    HttpVersion m_version;
    HttpMethod m_method;
    string m_path;
    string m_host;
    map<string,string> m_params;
    MemoryStream *m_body_stream;
    bool m_gzip;
    map<string,string> m_header;
    ProtoDataState m_in_state;
    long m_in_body_pos;
    long m_body_size;
    MemoryStream *m_in_stream;
    OnHttpProgress m_on_state_change;
    void *m_on_state_data;
    void Init();
public:
    virtual MemoryStream *ToStream();
    virtual ProtoDataState FromStream(const char *inStream, unsigned long sz);
public:
    FysHttpRequest();
    FysHttpRequest(const char *url);
    virtual ~FysHttpRequest();
public:
    void SetHeader(const char *k, const char *v);
    void SetUrlParam(const char *k, const char *v);
    void SetMethod(HttpMethod method);
    void SetPath(const char *path);
    void SetHost(const char *host);
    void SetGzip(bool gzip);
    bool WriteBody(char *buf, long bufSize);
    string GetHost();
    string GetPath(){return m_path;}
    string GetParam(const char *paramName);
    string GetHeader(const char *headName);
    HttpMethod GetMethod(){return m_method;}
    bool GetGzip(){return m_gzip;}
    void SetOnProgress(OnHttpProgress evt, void *data) {
        m_on_state_change = evt;
        m_on_state_data = data;
    }


    void Clean();
};

class FysHttpResponse {
private:
    FysHttpResponse(const FysHttpResponse&);
    FysHttpResponse& operator =(const FysHttpResponse&);
protected:
    HttpVersion m_version;
    MemoryStream *m_body_stream;
    bool m_gzip;
    map<string,string> m_header;
    ProtoDataState m_in_state;
    long m_in_body_pos;
    long m_body_size;
    MemoryStream *m_in_stream;
    int m_status_code;
    string m_status_reason;
    bool m_chunked;
    HttpChunkedDataState m_chunked_state;
    long m_chunk_size;
    string m_chunk_addition;
    OnHttpProgress m_on_state_change;
    void *m_on_state_data;

    void Init();
public:
    virtual MemoryStream *ToStream();
    virtual ProtoDataState FromStream(const char *inStream, unsigned long sz);
public:
    FysHttpResponse();
    virtual ~FysHttpResponse();
public:
    void SetHeader(const char *k, const char *v);
    void SetGzip(bool gzip);
    bool WriteBody(char *buf, long bufSize);
    string GetHeader(const char *headName);
    bool GetGzip(){return m_gzip;}
    int GetStatus(){return m_status_code;}
    void SetStatus(int code, const char *reason){m_status_code = code; m_status_reason = reason;}
    void SetOnStateChange(OnHttpProgress evt, void *data) {
        m_on_state_change = evt;
        m_on_state_data = data;
    }

    void Clean();
};

enum FysHttpConnectionState {
    hcsUnconnected,
    hcsConnected,
    hcsRequesting
};

//void _OnHttpClientDataRecved(TcpClient *c,void *buf, size_t len);
//void _OnHttpClientDataSent(TcpClient *c,void *buf, size_t len);
//void _OnHttpClientError(TcpClient *c, int errCode);
//void _OnHttpResponseState(void *data, const ProtoDataState state);

class FysHttpConnectionManager;
class FysHttpConnection {
    friend class FysHttpConnectionManager;
    friend void _OnHttpClientDataRecved(TcpClient *c,void *buf, size_t len);
    friend void _OnHttpClientDataSent(TcpClient *c,void *buf, size_t len);
    friend void _OnHttpClientError(TcpClient *c, int errCode);
    friend void _OnHttpResponseState(void *data, const ProtoDataState state);
private:
    FysHttpConnection(const FysHttpConnection&);
    FysHttpConnection& operator=(const FysHttpConnection&);
protected:
    FysHttpConnectionState m_state;
    string m_host;
    string m_ip;
    string m_port;
    TcpClient *m_tcpclient;
    deque<size_t> m_header_time; //millisecond
    timeval m_connect_timeout;
    size_t m_header_timeout; //millisecond
    pthread_mutex_t *m_sending_lock;
    SYSTEMTIME m_request_time;
    Pandc<int> *m_complete_chan; //P if response completed;
    FysHttpResponse *m_response;
    MemoryStream *m_request_stream;
    FysHttpConnectionManager *m_mgr;

    void SetHost(const char* host) {
        m_host = host;
    }
    void SetPort(const char *port) {
        m_port = port;
    }
    void SetIp(const char *ip) {
        m_ip = ip;
    }
    bool Connect();
    void SetHeaderTime(size_t duration);
public:
    FysHttpConnection();
    virtual ~FysHttpConnection();
public:
    string GetHost(){return m_host;}
    string GetIp(){return m_ip;}
    string GetPort(){return m_port;}
    FysHttpConnectionState GetState(){return m_state;}
    void AddHeaderTime(const SYSTEMTIME &tm);
    size_t GetHeaderTime();
    void SetConnectTimeout(const timeval *tv) {
        if (tv) {
            m_connect_timeout.tv_sec = tv->tv_sec;
            m_connect_timeout.tv_usec = tv->tv_usec;
        }
    }
    void SetHeaderTimeout(size_t t) {
        m_header_timeout = t;
    }
    int SendRequest(FysHttpRequest *req, FysHttpResponse *res);
};

class FysHttpConnectionManager: public TThread {
public:
    enum PickType {
        FirstIp,
        SpeedFirst,
        Rotate
    };
private:
    FysHttpConnectionManager(const FysHttpConnectionManager&);
    FysHttpConnectionManager& operator=(const FysHttpConnectionManager&);
private:
    map<string, vector<string>* > m_dns; //domain:ip
    map<string, map<string, vector<FysHttpConnection*>*>* > m_conections; //domain: ip: httpconnection
    map<string, size_t> m_pool_capacity; //domain: pool-size
    map<string, size_t> m_ip_index; //domain: ip-index
    pthread_mutex_t *m_connections_lock;
    PickType m_picktype;

    void LookupDomain(const char *domain, const char *port, bool clean);
protected:
    void Run();
public:
    FysHttpConnectionManager();
    virtual ~FysHttpConnectionManager();

    FysHttpConnection* PullConnection(const char *domain, const char *port, const timeval *tv);
    void PushConnection(FysHttpConnection *conn);
    void SetConnectionPoolCap(const char *domain, const char *port, size_t value) {
        m_pool_capacity[Fmt("%s:%s", domain,port)] = value;
    }
    void SetPickType(PickType tp) {
        m_picktype = tp;
    }
};



TcpClientManager *GetTcpClientManager();






#endif /* HTTP_HTTP_TYPES_H_ */
