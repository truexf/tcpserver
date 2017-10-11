/*
 * response.cpp
 *
 *  Created on: Sep 6, 2017
 *      Author: root
 */

#include "http_types.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/gzip.h"

void FysHttpResponse::Init() {
/*
    HttpVersion m_version;
    MemoryStream *m_body_stream;
    bool m_gzip;
    map<string,string> m_header;
    ProtoDataState m_in_state;
    long m_in_body_pos;
    long m_body_size;
    MemoryStream *m_in_stream;
 * */
    m_version = hv11;
    m_body_stream = NULL;
    m_body_size = 0;
    m_gzip = false;
    m_header.clear();
    m_in_state = rsHeaderUncompleted;
    m_in_body_pos = -1;
    m_in_stream = NULL;
    m_status_code = 200;
    m_status_reason = "OK";
    m_chunked = false;
    m_chunked_state = hcdsUnknown;
    m_chunk_size = 0;
    m_on_state_change = NULL;
    m_on_state_data = NULL;
}
MemoryStream* FysHttpResponse::ToStream(MemoryStream *stream) {
    MemoryStream *stm = stream;
    if (!stm)
        stm = new MemoryStream();
    stm->Empty();
    string ver;
    if (hv10 == m_version)
        ver = "HTTP/1.0";
    else
        ver = "HTTP/1.1";
    string ln = FormatString("%s %d %s\r\n", ver.c_str(), m_status_code, m_status_reason.c_str());
    stm->Write(ln.c_str(),ln.length());
    for(map<string,string>::iterator it = m_header.begin(); it != m_header.end(); it++) {
        ln = FormatString("%s: %s\r\n", it->first.c_str(), it->second.c_str());
        stm->Write(ln.c_str(),ln.length());
    }
    stm->Write("\r\n",2);
    if (m_body_stream && m_body_stream->GetSize() > 0) {
        if (!m_gzip) {
            ln = FormatString("%s: %d\r\n", "Content-Length", m_body_stream->GetSize());
            stm->Write(ln.c_str(),ln.length());
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
                stm->Write(zipOut, zipOutSize);
                free(zipOut);
            }
        }
    }

    return stm;
}

ProtoDataState FysHttpResponse::FromStream(const char* inStream, unsigned long sz) {
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
        if ((m_in_stream->GetSize() >=9 && memcmp(m_in_stream->GetBuffer(), "HTTP/1.0 ", 9) != 0) ||
                (m_in_stream->GetSize() >= 9 && memcmp(m_in_stream->GetBuffer(), "HTTP/1.1", 9) != 0)) {
            m_in_state = rsErrorProto;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
            return rsErrorProto;
        }
        if (-1 == m_in_body_pos) {
            m_in_state = rsHeaderUncompleted;
            if (m_on_state_change)
                m_on_state_change(m_on_state_data, m_in_state);
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
            size_t verPos = headerLines[0].find("HTTP/1.0 ");
            if (verPos != string::npos) {
                m_version = hv10;
            } else {
                verPos = headerLines[0].find("HTTP/1.1 ");
                if (verPos != string::npos)
                    m_version = hv11;
                else {
                    m_in_state = rsErrorProto;
                    if (m_on_state_change)
                        m_on_state_change(m_on_state_data, m_in_state);
                    return rsErrorProto;
                }
            }
            vector<string> line1;
            SplitString(headerLines[0]," ",line1);
            if (line1.size() < 2) {
                m_in_state = rsErrorProto;
                if (m_on_state_change)
                    m_on_state_change(m_on_state_data, m_in_state);
                return rsErrorProto;
            }
            m_status_code = atoi(line1[1].c_str());
            if (line1.size() > 2) {
                m_status_reason = line1[2];
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

            m_body_size = 0;
            map<string,string>::const_iterator itHeader = m_header.find("Content-Length");
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

            m_chunked = false;
            itHeader = m_header.find("Transfer-Encoding");
            if (itHeader != m_header.end() && strcasestr(itHeader->second.c_str(),"chunked") != NULL) {
                m_chunked = true;
                m_chunked_state = hcdsChunkHeaderUncompleted;
                m_chunk_size = 0;
            }

            if (m_body_size == 0 && !m_chunked) {
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
        if (!m_chunked) {
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
        } else {
            //chunked
chunk:
            if (hcdsChunkHeaderUncompleted == m_chunked_state) {
                for (long i = m_in_body_pos; i<=m_body_size-2; i++) {
                    void *buf = AddPtr(m_in_stream->GetBuffer(), m_in_body_pos);
                    if (memcmp(buf,"\r\n",2) == 0) {
                        m_chunked_state = hcdsChunkBodyUncompleted;
                        m_chunk_size = strtol((const char*)buf,NULL,16);
                        if (LONG_MIN == m_chunk_size ||LONG_MAX == m_chunk_size) {
                            m_in_state = rsErrorProto;
                            if (m_on_state_change)
                                m_on_state_change(m_on_state_data, m_in_state);
                            return rsErrorProto;
                        }
                        if (m_chunk_size > 0) {
                            m_in_body_pos = i+2;
                            goto chunk;
                        } else if (m_chunk_size < 0) {
                            m_in_state = rsErrorProto;
                            if (m_on_state_change)
                                m_on_state_change(m_on_state_data, m_in_state);
                            return rsErrorProto;
                        } else {
                            m_in_body_pos = i+2;
                            m_chunked_state = hcdsChunkEnding;
                            goto chunk;
                        }
                    }
                }
                if (m_body_size - m_in_body_pos > 18) {
                    m_in_state = rsErrorProto;
                    if (m_on_state_change)
                        m_on_state_change(m_on_state_data, m_in_state);
                    return rsErrorProto;
                } else {
                    return m_in_state;
                }
            }
            if (hcdsChunkBodyUncompleted == m_chunked_state) {
                if (m_in_stream->GetSize() - m_in_body_pos < m_chunk_size+2)
                    return m_in_state;
                void *buf = AddPtr(m_in_stream->GetBuffer(), m_in_body_pos);
                m_body_stream->Write(buf,m_chunk_size);
                m_chunked_state =hcdsChunkHeaderUncompleted;
                m_in_body_pos += m_chunk_size+2;
                if (m_in_stream->GetSize() > m_in_body_pos)
                    goto chunk;
            }
            if (hcdsChunkEnding == m_chunked_state) {
                long retain = m_in_stream->GetSize() - m_in_body_pos;
                if (retain >=2) {
                    void *buf = AddPtr(m_in_stream->GetBuffer(), m_in_body_pos);
                    long chunkEnd = MemFind((const char*)buf,retain,"\r\n",2,false);
                    if (chunkEnd != -1) {
                        m_chunked_state = hcdsChunkEnding;
                        m_in_state = rsCompleted;
                        if (m_on_state_change)
                            m_on_state_change(m_on_state_data, m_in_state);
                        m_chunk_addition = string((const char*)buf,chunkEnd);
                        return rsCompleted;
                    }
                }
                if (retain>1024) {
                    m_in_state = rsErrorProto;
                    if (m_on_state_change)
                        m_on_state_change(m_on_state_data, m_in_state);
                    return m_in_state;
                }
            }
        }
        return m_in_state;
    }
    if (rsCompleted ==m_in_state) {
        m_in_state = rsErrorProto;
        if (m_on_state_change)
            m_on_state_change(m_on_state_data, m_in_state);
        return rsErrorProto;
    }
    return m_in_state;
}

FysHttpResponse::FysHttpResponse() {
    Init();
}
FysHttpResponse::~FysHttpResponse() {
    if (m_body_stream)
        delete m_body_stream;
    if (m_in_stream)
        delete m_in_stream;
}

string FysHttpResponse::GetHeader(const char* headName) {
    map<string,string>::const_iterator it = m_header.find(headName);
    if (it != m_header.end())
        return it->second;

    return "";
}

void FysHttpResponse::Clean() {
    m_version = hv11;
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
    m_status_code = 200;
    m_status_reason = "OK";
    m_chunked = false;
    m_chunked_state = hcdsUnknown;
    m_chunk_size = 0;
    m_chunk_addition.clear();
    m_on_state_change = NULL;
    m_on_state_data = NULL;
}

void FysHttpResponse::SetHeader(const char *k, const char *v) {
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

void FysHttpResponse::SetGzip(bool gzip) {
    m_gzip = gzip;
}

bool FysHttpResponse::WriteBody(char *buf, long bufSize) {
    if (!buf || bufSize <= 0) {
        return false;
    }
    if (!m_body_stream) {
        m_body_stream = new MemoryStream;
    }
    return m_body_stream->Write(buf,bufSize);
}








