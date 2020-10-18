#pragma once
#include "noncopyable.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Buffer.h"
#include <memory>
#include <string>
#include <atomic>
class Channel;
class EventLoop;
class Socket;

/**
 *  TcpServer => Accepter => accept拿到connfd
 *  => TcpConnection 设置回调 => Channel => Poller => 执行Channel的回调
*/
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);

    ~TcpConnection();
    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }
    bool disConnected() const { return state_ == kDisConnected; }

    // 发送数据
    void send(const void *message, int len);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb) {
        highWaterMarkCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    // 连接建立和销毁
    void connectEstablished();
    void connectDestroyed();
    
    void shutdown();
private:
    enum StateE { kDisConnected, kConnecting, kConnected, kDisConnecting};
    void setState(StateE state);
    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void send(const std::string &buf);
    
    void sendInLoop(const void* message, size_t len);

    
    void shutdownInLoop();

    EventLoop *loop_; // 这里绝对不是mainLoop
    const std::string name_;
    std::atomic_int state_;

    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    // 用户在TcpServer设置 => TcpConnection => Channel调用
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_:
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;

    // 读和写的缓存
    Buffer inputBuffer_;
    Buffer outputBuffer_;

};
