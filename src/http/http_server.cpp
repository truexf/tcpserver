/*
 * http_server.cpp
 *
 *  Created on: Oct 11, 2017
 *      Author: root
 */

#include "http_types.h"

FysHttpServer::FysHttpServer() {
    m_tcpserver = NULL;
    m_service_mux_lock = CreateMutex(false);
}

FysHttpServer::~FysHttpServer() {

}

struct _http_server_client_data {
    FysHttpRequest *request;
    FysHttpResponse *response;
    FysHttpServer *server;
    MemoryStream *responseBuf;
};

void _on_http_server_client_recved(Client *c,void *buf,size_t len) {
    _http_server_client_data *data = (_http_server_client_data*)(c->m_data);
    ProtoDataState stat = data->request->FromStream((const char*)buf,(size_t)len);
    if (rsErrorProto == stat) {
        c->Close();
        return;
    } else if (rsCompleted == stat) {
        FysHttpServer *svr = (FysHttpServer*)(c->m_svr->m_data);
        FysHttpHandler *handler = svr->FindMux(data->request->GetPath());
        if (handler) {
            try {
                if (0 == handler->Execute(data->request,data->response)) {
                    data->responseBuf = data->response->ToStream(data->responseBuf);
                    data->request->Clean();
                    data->response->Clean();
                    c->Send(data->responseBuf->GetBuffer(), data->responseBuf->GetSize(),0);
                    return;
                } else {
                    data->response->Clean();
                    data->response->SetStatus(404,"resource not found");
                    data->responseBuf = data->response->ToStream(data->responseBuf);
                    data->request->Clean();
                    data->response->Clean();
                    c->Send(data->responseBuf->GetBuffer(), data->responseBuf->GetSize(),0);
                    return;
                }
            } catch(...) {
                data->request->Clean();
                data->response->Clean();
                data->response->SetStatus(500,"internal server error");
                data->responseBuf = data->response->ToStream(data->responseBuf);
                c->Send(data->responseBuf->GetBuffer(), data->responseBuf->GetSize(),0);
                return;
            }
        }
    }
}

void _on_http_server_client_sent(Client *c,void *buf,size_t len,bool success ) {

}

void _on_http_server_client_connected(Client *clt) {
    clt->m_on_recved = _on_http_server_client_recved;
    clt->m_on_sent = _on_http_server_client_sent;
    FysHttpRequest *req = new FysHttpRequest(); // todo: maybe from pool
    FysHttpResponse *res = new FysHttpResponse();
    _http_server_client_data *data = new _http_server_client_data;
    data->request = req;
    data->response = res;
    data->server = (FysHttpServer*)(clt->m_svr->m_data);
    data->responseBuf = NULL;
    clt->SetData(data);
}
void _on_http_server_client_disconnected(Client *clt) {
    _http_server_client_data *data = (_http_server_client_data *)(clt->m_data);
    if (data) {
        if (data->request)
            delete data->request; //todo: maybe push to pool;
        if (data->response)
            delete data->response;
        if (data->responseBuf) {
            delete data->responseBuf;
        }
        delete data;
    }
}

void _on_http_server_error(int err_code, const char* err_msg) {
    //log something
}

void FysHttpServer::RegisterMux(string path, FysHttpHandler *handler) {
    if (path.empty())
        return;
    AutoMutex auto1(m_service_mux_lock);
    map<string, FysHttpHandler*>::iterator it = m_service_mux.find(path);
    if (!handler && it != m_service_mux.end()) {
        m_service_mux.erase(it);
        return;
    }
    m_service_mux[path] = handler;
}

FysHttpHandler *FysHttpServer::FindMux(string path) {
    AutoMutex auto1(m_service_mux_lock);
    map<string, FysHttpHandler*>::iterator it = m_service_mux.find(path);
    if (it != m_service_mux.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

bool FysHttpServer::Start(string configFile, XLog *log) {
    m_tcpserver = new TcpServer(configFile, log);
    m_tcpserver->m_data = this;
    m_tcpserver->m_on_client_connected = _on_http_server_client_connected;
    m_tcpserver->m_on_client_disconnected = _on_http_server_client_disconnected;
    m_tcpserver->m_on_error = _on_http_server_error;
    return m_tcpserver->Startup(-1);
}
