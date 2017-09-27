/*
 * xsocketutils.cpp
 *
 *  Created on: Sep 27, 2017
 *      Author: root
 */

#include "xsocketutils.hpp"
#include "errno.h"
#include <vector>
#include <string>
using std::string;
using std::vector;

int SetSocketNonblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

int TcpBind(int sockfd, const char *ip, unsigned short port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    return bind(sockfd, (sockaddr*) &addr, sizeof(sockaddr_in));
}

void SetSocketKeepalive(int sockfd, int keepalive, int idle_seconds)
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

int SetSocketLinger(int sockfd, int l_onoff, int l_linger)
{
    linger lg;
    lg.l_onoff = l_onoff;
    lg.l_linger = l_linger;
    return setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lg, sizeof(linger));
}

int AcceptTimeout(int listensockfd, struct timeval *tv, struct sockaddr *addr, socklen_t *addrlen) {
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(listensockfd, &rfd);
    int nfds = select(listensockfd+1, &rfd, NULL, NULL, tv);
    if (nfds > 0)
        return accept(listensockfd, addr, addrlen);
    else
        return nfds;
}

//return 0 success, -1 error, -2 timeout
int TcpConnectTimeout(int sockfd, struct timeval *tv, const struct sockaddr *addr,socklen_t addrlen) {
    if (sockfd <= 0)
        return -1;
    if (!tv) {
        return connect(sockfd, addr, addrlen);
    }
    SetSocketNonblock(sockfd);
    int ret = -1;
    int connected = connect(sockfd, (struct sockaddr *) &addr, addrlen);
    if (connected == 0 ) {
        return 0;
    } else if(errno != EINPROGRESS) {
            return -1;
    } else {
        fd_set set,rset;
        FD_ZERO(&set);
        FD_ZERO(&rset);
        FD_SET(sockfd,&set);
        FD_SET(sockfd,&rset);
        int res = select(sockfd+1,&rset,&set,NULL,tv);
        if(res < 0) {
            ret =  -1;
        } else if (res == 0) {
            ret = -2;
        } else if (1 == res) {
            return 0;
        } else {
            return -3;
        }
    }
    return -1;
}

int Ipv4Addr1(const char *domain,const char* serviceOrPort,sockaddr_in *ret) {
    if (!ret || !domain)
        return -1;
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_protocol = 0;
    hints.ai_addr = NULL;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    hints.ai_flags = 0;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *addrp;
    int i = getaddrinfo(domain,serviceOrPort,&hints,&addrp);
    if (i) {
        return i;
    } else {
        memcpy(ret,addrp->ai_addr,sizeof(sockaddr_in));
        freeaddrinfo(addrp);
        return 0;
    }
}

int Ipv4AddrList(const char *domain,const char* serviceOrPort,sockaddr_in **ret, int *retCount) {
    if (!ret || !domain)
        return -1;
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_protocol = 0;
    hints.ai_addr = NULL;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    hints.ai_flags = 0;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *addrp;
    int iret = getaddrinfo(domain,serviceOrPort,&hints,&addrp);
    if (iret) {
        return iret;
    } else {
        *retCount = 0;
        for(addrinfo *rp = addrp; rp != NULL; rp = rp->ai_next) {
            *retCount = 1 + *retCount;
        }
        *ret = (sockaddr_in*)malloc(*retCount * sizeof(sockaddr_in));
        sockaddr_in *buf = *ret;
        for(addrinfo *rp = addrp; rp != NULL; rp = rp->ai_next) {
            memcpy(buf,addrp->ai_addr,sizeof(sockaddr_in));
            buf++;
        }

        freeaddrinfo(addrp);
        return 0;
    }
}

int DnsLookup(const char *domain, vector<string> &ret) {
    sockaddr_in *ips;
    int ipCount=0;
    if (-1 == Ipv4AddrList(domain,NULL,&ips,&ipCount)) {
        return -1;
    }
    ret.clear();
    sockaddr_in *ip = ips;
    char ipStr[16];
    for (int i = 0; i<ipCount; i++) {
        ip += i;
        memset(ipStr,0,sizeof(ipStr));
        inet_ntop(AF_INET,&ip->sin_addr,ipStr,sizeof(ipStr));
        ret.push_back(ipStr);
    }
    FreeIpv4List(ips);
    return ipCount;
}


