#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0) {
        LOG_FATAL("%s:%s:%d listen socket create err:%d n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
     : loop_(loop)
     , acceptSocket_(createNonblocking())
     , acceptChannel_(loop, acceptSocket_.fd())
     , listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    // tcpServer::start() ==>Acceptor.listen 有新用户连接就执行回调, connfd打包Channel给subLoop
    // 注册回调 
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();

}

// listenfd收到读事件, 有新用户连接
void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0) {
        if(newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        }else {
            ::close(connfd);
        }
    }else {
        LOG_ERROR("%s:%s:%d accept err:%d n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE) {
            // 进程能打开的文件描述符达到上限
            // 调整此上限, 或者服务器扩容
        }
    }
}