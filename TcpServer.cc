#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <functional>
#include <strings.h>

static EventLoop * CheckLoopNotNull(EventLoop *loop) {
    if(loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option) : 
                     loop_(loop)
                     , ipPort_(listenAddr.toIpPort)
                     , name_(nameArg)
                     , acceptor_(new Acceptor(loop, listenAddr, option == KReusePort))
                     , threadPool_(new EventLoopThreadPool(loop, name_))
                     , connectionCallback_(defaultConnectionCallback)
                     , messgaeCallback_(defaultMessageCallback) {
    // 有新用户连接时会在handleRead()里执行此回调 传入connfd peerAddr
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                        std::placeholders::_1, std::placeholders::_2));
                                        
}
TcpServer::~TcpServer() {
    for(auto &item : connections_) {
        TcpConnectionPtr conn(item.second); // 这个局部的shared_ptr出右括号, 可以自动释放new出来的TcpConnection对象
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}
// 设置subLoop个数
void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听 loop_.loop()
void TcpServer::start() {

    // 防止被重复启动
    // 启动底层的loop线程池
    if(started_++ == 0) {
        threadPool_->start(threadInitCalback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 主线程的mainLoop调用此函数
// 根据轮询算法选择一个subLoop
// 唤醒subLoop
// 当前connfd封装成channel分发给subLoop
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    EventLoop *ioLoop = threadPool_->getNextLoop();
    // muduo没用生产者消费者模型, 而是使用wakeupfd
    // 子线程的回调函数不能在主线程执行
    // runInLoop or queueInLoop
    char buf[64];

    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] -- new conn [%s] from %s \n", 
        name_.c_str(), connName.c_str(), peerAddr.toIpPort.c_str());
    // 通过sockfd获取其绑定的本机的ip地址和端口
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;

    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0) {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);
    // 根据连接成功的sockfd, 创建connection对象
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr
    ));

    connections_[connName] = conn;

    // 用户设置给TcpServer -> TcpConnection -> Channel
    // poller -> notify
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    // 直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));

}
void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );

}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection \n", name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}