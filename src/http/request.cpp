#include "http_types.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/gzip.h"

void FysHttpRequest::Init() {
/*
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
 * */
    m_version = hv11;
    m_method = hmGet;
    m_host.clear();
    m_path = "/";
    m_params.clear();
    m_body_stream = NULL;
    m_body_size = 0;
    m_gzip = false;
    m_header.clear();
    m_in_state = rsHeaderUncompleted;
    m_in_body_pos = -1;
    m_in_stream = NULL;
    m_on_state_change = NULL;
    m_on_state_data = NULL;
}
MemoryStream* FysHttpRequest::ToStream() {
    MemoryStream *stm = new MemoryStream();
    string ver;
    if (hv10 == m_version)
        ver = "HTTP/1.0";
    else
        ver = "HTTP/1.1";
    if (hmGet == m_method) {
        string ln;
        if (m_params.empty())
            ln = FormatString("GET %s %s\r\n",m_path.c_str(),ver.c_str());
        else
            ln = FormatString("GET %s?%s %s\r\n",m_path.c_str(),CombineUrlParams(m_params).c_str(),ver.c_str());
        stm->Write(ln.c_str(),ln.length());
        ln = FormatString("Host: %s\r\n", m_host.c_str());
        stm->Write(ln.c_str(),ln.length());
        for(map<string,string>::iterator it = m_header.begin(); it != m_header.end(); it++) {
            ln = FormatString("%s: %s\r\n", it->first.c_str(), it->second.c_str());
            stm->Write(ln.c_str(),ln.length());
        }
        stm->Write("\r\n",2);
        return stm;
    } else { //post
        string ln(FormatString("POST %s %s\r\n",m_path.c_str(),ver.c_str()));
        stm->Write(ln.c_str(),ln.length());
        ln = FormatString("Host: %s\r\n", m_host.c_str());
        stm->Write(ln.c_str(),ln.length());
        for(map<string,string>::iterator it = m_header.begin(); it != m_header.end(); it++) {
            ln = FormatString("%s: %s\r\n", it->first.c_str(), it->second.c_str());
            stm->Write(ln.c_str(),ln.length());
        }
        if (m_body_stream && m_body_stream->GetSize() > 0) {
            if (!m_gzip) {
                ln = FormatString("%s: %d\r\n", "Content-Length", m_body_stream->GetSize());
                stm->Write(ln.c_str(),ln.length());
                stm->Write("\r\n",2);
                stm->Write(m_body_stream->GetBuffer(),m_body_stream->GetSize());
            } else {
                unsigned char* zipOut = (unsigned char*)malloc(m_body_stream->GetSize());
                if (!zipOut) {
                    delete stm;
                    return NULL;
                }
                unsigned long zipOutSize = m_body_stream->GetSize();
                if (0 != gzcompress((unsigned char*)m_body_stream->GetBuffer(),(unsigned long)m_body_stream->GetSize(),zipOut,&zipOutSize)) {
                    delete stm;
                    return NULL;
                } else {
                    ln = FormatString("Content-Encoding: gzip\r\n%s: %d\r\n", "Content-Length", zipOutSize);
                    stm->Write(ln.c_str(),ln.length());
                    stm->Write("\r\n",2);
                    stm->Write(zipOut, zipOutSize);
                    free(zipOut);
                }
            }
        }
    }
    return stm;
}

ProtoDataState FysHttpRequest::FromStream(const char* inStream, unsigned long sz) {
    if (rsErrorProto == m_in_state)
        return rsErrorProto;
    if (!m_in_stream)
        m_in_stream = new MemoryStream;
    m_in_stream->Write(inStream, sz);
loop:
    if (rsHeaderUncompleted == m_in_state) {
        m_in_body_pos = m_in_stream->Find("\r\n\r\n",4, false);
        //header uncompleted, size > 8K, i think this is an invalid proto data
        if (-1 == m_in_body_pos && m_in_stream->GetSize() > 8192) {
            m_in_state = rsErrorProto;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            return rsErrorProto;
        }
        if ((m_in_stream->GetSize() >=3 && memcmp(m_in_stream->GetBuffer(), "GET", 3) != 0) ||
                (m_in_stream->GetSize() >= 4 && memcmp(m_in_stream->GetBuffer(), "POST", 4) != 0)) {
            m_in_state = rsErrorProto;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            return rsErrorProto;
        }
        if (-1 == m_in_body_pos) {
            m_in_state = rsHeaderUncompleted;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, rsHeaderUncompleted);
            return rsHeaderUncompleted;
        } else {
            m_in_state = rsHeaderCompleted;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            m_in_state = rsBodyUncompleted;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            //parse header
            vector<string> headerLines;
            SplitString((const char*)m_in_stream->GetBuffer(),m_in_body_pos,"\r\n",2,headerLines);
            size_t verPos = headerLines[0].rfind(" HTTP/1.0");
            if (verPos != string::npos) {
                m_version = hv10;
            } else {
                verPos = headerLines[0].rfind(" HTTP/1.1");
                if (verPos != string::npos)
                    m_version = hv11;
                else {
                    m_in_state = rsErrorProto;
                    if (m_on_state_change)
                        m_on_state_change(m_on_state_data, m_in_state);
                    return rsErrorProto;
                }
            }
            if (headerLines[0].find("GET") != string::npos) {
                m_method = hmGet;
            } else {
                m_method = hmPost;
            }
            size_t pathPos = headerLines[0].find("/");
            if (pathPos == string::npos) {
                m_in_state = rsErrorProto;
                return rsErrorProto;
            }
            size_t paramPos = 0;
            m_params.clear();
            for (size_t i = pathPos; i < verPos; i++) {
                if (headerLines[0][i] == '?') {
                    paramPos = i+1;
                    m_path = headerLines[0].substr(pathPos,i-pathPos);
                    const char* buf = headerLines[0].c_str() + paramPos;
                    BreakUrlParam(buf,verPos - paramPos,m_params);
                    break;
                }
            }
            m_header.clear();
            for (size_t i = 1; i<headerLines.size(); i++) {
                string L,R;
                SplitString(headerLines[i],":",L,R);
                L = trim(L);
                if (!L.empty()) {
                    m_header[NormalizeHttpHeaderKey(L.c_str())] = trim(R);
                }
            }
            map<string, string>::const_iterator itHeader = m_header.find("Host");
            if (itHeader != m_header.end()) {
                m_host = itHeader->second;
            }
            if (hmPost == m_method) {
                m_body_size = 0;
                itHeader = m_header.find("Content-Length");
                if (itHeader != m_header.end()) {
                    m_body_size = atol(itHeader->second.c_str());
                }
                itHeader = m_header.find("Content-Encoding");
                if (itHeader != m_header.end()) {
                    if (strcasestr(itHeader->second.c_str(),"gzip") == NULL) {
                        m_gzip = false;
                    } else {
                        m_gzip = true;
                    }
                }
                if (m_body_size == 0) {
                    m_in_state = rsCompleted;
                    if (m_on_state_change)
                        m_on_state_change(m_on_state_data, m_in_state);
                    return rsCompleted;
                }
            } else {
                m_in_state = rsCompleted;
                if (m_on_state_change)
                    m_on_state_change(m_on_state_data, m_in_state);
                return rsCompleted;
            }
            m_body_stream->Clear();
            goto loop;
        }
    }
    if (rsBodyUncompleted == m_in_state) {
        void *buf = AddPtr(m_in_stream->GetBuffer(), m_in_body_pos);
        long sz = std::min(m_body_size,m_in_stream->GetSize()-m_in_body_pos);
        if (sz == m_body_size) {
            m_body_stream->Write(buf,sz);
            m_in_state = rsCompleted;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            return rsCompleted;
        } else {
            m_in_body_pos += sz;
            return rsBodyUncompleted;
        }
    }
    if (rsCompleted ==m_in_state) {
        m_in_state = rsErrorProto;
        if (m_on_state_change)
            m_on_state_change(m_on_state_data, m_in_state);
        return rsErrorProto;
    }
    return m_in_state;
}

FysHttpRequest::FysHttpRequest() {
    Init();
}
FysHttpRequest::FysHttpRequest(const char *url) {
    Init();
    fyslib::UrlInfo info;
    if (fyslib::ParseUrl(url,&info)) {
        if (info.proto != "http") {
            m_in_state = rsErrorProto;
            return;
        }
        if (!info.port.empty() && info.port != "80")
            m_host = info.domain+":"+info.port;
        else
            m_host = info.domain;
        m_path = info.path;
        m_params.clear();
        for (map<string,string>::const_iterator it = info.params.begin(); it != info.params.end(); it++) {
            m_params[it->first] = it->second;
        }
    }
}
FysHttpRequest::~FysHttpRequest() {
    if (m_body_stream)
        delete m_body_stream;
    if (m_in_stream)
        delete m_in_stream;
}

string FysHttpRequest::GetParam(const char* paramName) {
    map<string,string>::const_iterator it = m_params.find(paramName);
    if (it !=m_params.end()) {
        return it->second;
    }
    return "";
}

string FysHttpRequest::GetHeader(const char* headName) {
    map<string,string>::const_iterator it = m_header.find(headName);
    if (it != m_header.end())
        return it->second;

    return "";
}

void FysHttpRequest::Clean() {
    m_version = hv11;
    m_method = hmGet;
    m_host.clear();
    m_path = "/";
    m_params.clear();
    if (m_body_stream)
        m_body_stream->Clear();
    m_body_size = 0;
    m_gzip = false;
    m_header.clear();
    m_in_state = rsHeaderUncompleted;
    m_in_body_pos = -1;
    if (m_in_stream) {
        m_in_stream->Clear();
    }
    m_on_state_change = NULL;
    m_on_state_data = NULL;
}

void FysHttpRequest::SetHeader(const char *k, const char *v) {
    if (!k) {
        return;
    }
    string sk(NormalizeHttpHeaderKey(k));
    if (v) {
        m_header[sk] = string(v);
    } else {
        map<string,string>::iterator it = m_header.find(sk);
        if (it != m_header.end()) {
            m_header.erase(it);
        }
    }
}

void FysHttpRequest::SetUrlParam(const char *k, const char *v) {
    if (!k)
        return;
    string sk(k);
    if (v)  {
        m_params[sk] = string(v);
    } else {
        map<string,string>::iterator it = m_params.find(sk);
        if (it != m_params.end()) {
            m_params.erase(it);
        }
    }
}

void FysHttpRequest::SetMethod(HttpMethod method) {
    m_method = method;
}

void FysHttpRequest::SetPath(const char *path) {
    m_path = string(path);
    if (m_path.empty()) {
        m_path = "/";
    }
}

void FysHttpRequest::SetHost(const char *host) {
    if (host) {
        m_host = string(host);
    }
}

void FysHttpRequest::SetGzip(bool gzip) {
    m_gzip = gzip;
}

bool FysHttpRequest::WriteBody(char *buf, long bufSize) {
    if (!buf || bufSize <= 0) {
        return false;
    }
    if (!m_body_stream) {
        m_body_stream = new MemoryStream;
    }
    return m_body_stream->Write(buf,bufSize);
}

string FysHttpRequest::GetHost() {
    return m_host;
}



