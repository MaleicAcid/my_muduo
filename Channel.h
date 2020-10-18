#pragma once

#include "noncopyable.h"
#include <functional>
#include <memory>
#include "Timestamp.h"
class EventLoop; // 前置声明, 对客户暴露信息少
//class Timestamp;

// EventLoop = Poller + Channel
// Channel=fd+events+revents
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知后处理事件,调用回调函数
    void handleEvent(Timestamp receiveTime);

    // setter设置回调函数
    // 优先调用EventCallback的移动构造
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止channel没手动remove掉, channel还在执行回调
    void tie(const std::shared_ptr<void>&);
    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    // 让fd对读事件感兴趣, update()就是调用epoll_ctl
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_; // fd, Poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_; // poller返回的这个fd具体发生的事件
    int index_;

    std::weak_ptr<void> tie_; // weak_ptr用于监听跨线程资源的生存状态(实现弱回调)
    bool tied_;
    // 获知具体发生的事件后, channel负责事件回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};