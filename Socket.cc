#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
Socket::~Socket() {
    close(sockfd_);

}
void Socket::bindAddress(const InetAddress &localaddr) {
    if(0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr()), sizeof(sockaddr_in)) {
        LOG_FATAL("bind sockfd:%d failed", sockfd_);
    }
}
void Socket::listen() {
    if(0 != ::listen(sockfd_, 1024)) {
        LOG_FATAL("listen sockfd:%d failed", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr) {
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if(connfd >= 0) {
        // 有效的fd
        peeraddr->setSockAddr(addr);// 通过输出参数
    }

    return connfd;

}
void Socket::shutdownWrite() {
    // 只是关闭写端
    if(::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR("sockets::shutdown() error");
    }

}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELY, &optval, sizeof optval);
}
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}
void Socket::setKeepalive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}