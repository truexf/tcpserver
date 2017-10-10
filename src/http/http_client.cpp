/*
 * httpclient.cpp
 *
 *  Created on: Sep 5, 2017
 *      Author: root
 */

#include "http_types.h"

pthread_once_t _tcpclientManagerOnceControl = PTHREAD_ONCE_INIT;
TcpClientManager *_HttpConnectionManager = NULL;
void _InitTcpclientManager() {
    _HttpConnectionManager = new TcpClientManager;
    _HttpConnectionManager->Init(_log,3);
    _HttpConnectionManager->Go();
}

TcpClientManager *GetTcpClientManager() {
    return _HttpConnectionManager;
}





