#pragma once
#include <vector>
#include <sys/epoll.h>
#include "Timestamp.h"

class Poller;
class Channel;

/*
①epoll_create
②epoll_ctl add/mod/del
③epoll_wait

*/
class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop *loop); // ①epoll_create
    ~EpollPoller() override;

    // 虚函数重写
    Timestamp poll(int timeousMs, ChannelList *activeChannels) override; // ③epoll_wait
    void updateChannel(Channel *channel) override; // ②epoll_ctl add/mod
    void removeChannel(Channel *channel) override; // ②epoll_ctl del

private:
    static const int kIntEventListSize = 16;
    // 填写活跃的链接
    void fillActiveChannels(int numEvents, ChannelLists *activeChannels) const;
    void update(int operation, Channel *channel); // epoll_ctl
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};