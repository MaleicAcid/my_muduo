#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <errno>
#include <functional>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

static EventLoop * CheckLoopNotNull(EventLoop *loop) {
    if(loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                            const std::string &name,
                            int sockfd,
                            const InetAddress& localAddr,
                            const InetAddress& peerAddr)
                            : loop_(CheckLoopNotNull(loop))
                            , name_(nameArg)
                            , state_(kConnecting)
                            , reading_(true)
                            , socket_(new Socket(sockfd))
                            , channel_(new Channel(loop, sockfd))
                            , localAddr_(localAddr)
                            , peerAddr_(peerAddr)
                            , highWaterMark_(64*1024*1024) { // 64M
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));    
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));    
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));    
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepalive(true); // muduo默认开启tcp保活
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::handleRead(TimeStamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0) {
        // 已建立连接的用户, 有可读事件发生了
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }else if(n == 0) {
        handleClose();
    }else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if(channel_->isWriting()) {
        int savedError = 0;
        size_t n = outputBuffer_.writeFd(channel_->fd(), &savedError);
        if(n > 0) {
            outputBuffer_.retrieve(n); // 序列化
            if(outputBuffer_.readableBytes() == 0) {
                // 已经发送完成
                channel_->disableWriting();
                if(writeCompleteCallback_) {
                    // 唤醒loop_对应的thread线程执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if(state_ == kDisConnecting) {
                    shutdownInLoop();// 在loop中关闭这个TcpConnection
                }
            }
        }else{
            LOG_ERROR("TcpConnection::handleWrite");
        }

    }else {
        // channel 不可写
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());

    }
}
void TcpConnection::handleClose() {
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisConnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    // 没判断就直接调用了
    connectionCallback_(connPtr);
    closeCallback_(connPtr) // 由tcpServer执行
}
void TcpConnection::handleError() {
    int optval;
    socklen_t oplen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err =  errno;
    }else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}


void TcpConnection::send(const std::string &buf) {
    if(state_ == kConnected) {
        if (loop_ -> isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        }else {
            loop_->runInLoop(std::bind(
                &TcpConnection,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

// 应用写的快, 内核发生数据慢, 需要把待发送数据写入缓冲区
// 而且设置了水位回调
void TcpConnection::sendInLoop(const void* data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection的shutdown
    if(state_ == kDisConnecting) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // channel第一次开始写数据, 而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0) {
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_) {
                // 既然在这里数据全部发送完成, 就不用再给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }else { // nwrote<0 出错
            nwrote = 0;
            if(errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET) { // sigpipe reset
                    // 对端socket重置
                    faultError = true;
                }
            }
        }
    }

    // 没全部发送出去, 剩余的数据需要保存到缓冲区
    // 注册epollout事件 epoll是LT模式, 会通知相应的sock调用handleWrite()
    // 直到发送缓冲区全部发送完
    if(!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && HighWaterMarkCallback_) {
            
            loop_ ->queueInLoop(
                std::bind(HighWaterMarkCallback_, shared_from_this(), oldLen + remaining)
            );
        }
        outputBuffer_.append((char*) data + nwrote, remaining);
        if(!channel_->isWriting()) {
            channel_->enableWriting(); // 注册channel的写事件, 否则poller不会给channel通知epoll
        }
    }
}
void TcpConnection::shutdown() {
    if(state_ == kConnected) {
        setState(kDisConnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );

    }
}
void TcpConnection::shutdownInLoop() {
    if(!channel_->isWriting()) { // 说明outputbuffer中的数据已经全部发送完成
        socket_->shutdownWrite(); // poller通知关闭事件, 回调handleClose
    }
}

// 连接建立和销毁
void TcpConnection::connectEstablished() {
    setState(kConnected);
    // 防止channel上层的connection对象被remove掉
    channel_->tie(shared_from_this()); // 这个指针指向的对象不会被释放
    channel_->enableReading(); // 注册channel的epollin

    connectionCallback_(shared_from_this());

}
void TcpConnection::connectDestroyed() {
    if(state_ == kConnected) {
        setState(kDisConnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();

}