/*
 * consts.hpp
 *
 *  Created on: Jan 4, 2017
 *      Author: root
 */

#ifndef CONSTS_HPP_
#define CONSTS_HPP_

namespace tcpserver{

const char * const CFG_SERVER_SECTION = "SERVER";
const char * const CFG_SERVER_HEARTBEAT = "heartbeat";
const int CFGV_DEFAULT_SERVER_HEARTBEAT = 1;
const char * const CFG_SERVER_HEARTBEAT_INTERVAL_SECOND = "heartbeat_interval_second";
const int CFGV_DEFAULT_SERVER_HEARTBEAT_INTERVAL_SECOND = 60;
const char * const CFG_SERVER_LINGER = "linger";
const int CFGV_DEFAULT_SERVER_LINGER = 0;
const char * const CFG_SERVER_LISTENQUEUE_LENGTH = "listenqueue_length";
const int CFGV_DEFAULT_SERVER_LISTENQUEUE_LENGTH = 50;
const char * const CFG_SERVER_SENDBUF_SIZE = "sendbuf_size";
const char * const CFG_SERVER_RECVBUF_SIZE = "recvbuf_size";
const char * const CFG_SERVER_BIND_IP = "bind_ip";
const char * const CFGV_DEFAULT_SERVER_BIND_IP = "0.0.0.0";
const char * const CFG_SERVER_LISTEN_PORT = "listen_port";
const unsigned short CFGV_DEFAULT_LISTEN_PORT = 8888;
const char * const CFG_SERVER_LOG_PATH = "log_path";
const char * const CFG_SERVER_LOG_QUEUE_LENGTH = "log_queue_length";
const char * const CFG_SERVER_WORKER_COUNT = "worker_count";
const int CFGV_DEFAULT_SERVER_WORKER_COUNT = 10;
const char * const CFG_SERVER_CONSUMER_COUNT = "worker_count";
const int CFGV_DEFAULT_SERVER_CONSUMER_COUNT = 10;

const int RECV_BUF_SIZE = 8192;

const int ERR_CREATE_LISTEN_SKT_FAIL = 10000;

}//end of namespace

#endif /* CONSTS_HPP_ */
