/*
 * xsocketutils.hpp
 *
 *  Created on: Apr 17, 2016
 *      Author: root
 */

#ifndef FYSLIB_XSOCKETUTILS_HPP_
#define FYSLIB_XSOCKETUTILS_HPP_

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <string>
using std::string;
using std::vector;

inline int CreateTCPSocket()
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

int SetSocketNonblock(int fd);

int TcpBind(int sockfd, const char *ip, unsigned short port);

void SetSocketKeepalive(int sockfd, int keepalive, int idle_seconds);

inline int SetSocketNagle(int sockfd, int nodelay)
{
	return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
}

int SetSocketLinger(int sockfd, int l_onoff, int l_linger);

inline int SetSocketReuseAddr(int sockfd, int reuse)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
}

inline int SetSocketRecvbufSize(int sockfd, int buf_size)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(int));
}

inline int SetSocketSendbufSize(int sockfd, int buf_size)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(int));
}

//return 0 success, -1 error, -2 timeouts
int TcpConnectTimeout(int sockfd, struct timeval *tv, const struct sockaddr *addr,socklen_t addrlen);

int AcceptTimeout(int listensockfd, struct timeval *tv,
		struct sockaddr *addr, socklen_t *addrlen);

int Ipv4Addr1(const char *domain,const char* serviceOrPort,sockaddr_in *ret);

int Ipv4AddrList(const char *domain,const char* serviceOrPort,sockaddr_in **ret, int *retCount);

inline void FreeIpv4List(sockaddr_in *addrList) {
    free(addrList);
}

int DnsLookup(const char *domain, vector<string> &ret);



#endif /* FYSLIB_XSOCKETUTILS_HPP_ */
