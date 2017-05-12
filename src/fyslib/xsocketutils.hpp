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
#include <string.h>

inline int CreateTCPSocket()
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

inline int SetSocketNonblock(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, flag);
}

inline int TcpBind(int sockfd, const char *ip, unsigned short port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	return bind(sockfd, (sockaddr*) &addr, sizeof(sockaddr_in));
}

inline void SetSocketKeepalive(int sockfd, int keepalive, int idle_seconds)
{
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
	if (keepalive) {
		setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_seconds,
				sizeof(int));
		int intvl = 5;
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl,
                sizeof(int));
	}
}

inline int SetSocketNagle(int sockfd, int nodelay)
{
	return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
}

inline int SetSocketLinger(int sockfd, int l_onoff, int l_linger)
{
	linger lg;
	lg.l_onoff = l_onoff;
	lg.l_linger = l_linger;
	return setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lg, sizeof(linger));
}

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

inline int AcceptTimeout(int listensockfd, struct timeval *tv,
		struct sockaddr *addr, socklen_t *addrlen)
{
	fd_set rfd;
	FD_ZERO(&rfd);
	FD_SET(listensockfd, &rfd);
	int nfds = select(listensockfd+1, &rfd, NULL, NULL, tv);
	if (nfds > 0)
		return accept(listensockfd, addr, addrlen);
	else
		return nfds;
}



#endif /* FYSLIB_XSOCKETUTILS_HPP_ */
