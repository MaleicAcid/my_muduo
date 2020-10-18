#pragma once
#include <vector>
#include <unordered_map>
#include "noncopyable.h"
#include "Timestamp.h"
class Channel;
class EventLoop;

class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;
    // 统一的对外接口
    virtual Timestamp poll(int timeoutMs, Channel *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    // 判断参数channel是否在当前poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop 通过此接口获取默认的io复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // key: fd
    // value: fd所属的channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_; // 定义所属的事件循环
};