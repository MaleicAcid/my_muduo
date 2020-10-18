#pragma once


// 用户使用muduo要包含很多头文件

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
// connection和channel有区别
// 
class TcpServer : noncopyable {

public:
    using ThreadInitCalback = std::function<void(EventLoop*)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    enum Option {
        KNoReusePort,
        KReusePort,
    };
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = KNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ThreadInitCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const ThreadInitCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const ThreadInitCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置subLoop个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();
private:

    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; // 运行在main loop

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread
    // 用户传给tcpserver ==》 eventloop
    ConnectionCallback connectionCallback_; //有新链接
    MessageCallback messageCallback_; // 有读写消息
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完

    ThreadInitCalback threadInitCalback_; // loop线程初始化的回调
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接

};