#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <strings.h>
// channel.index 表示 channel 状态
const int kNew = -1; // channel.index 初始化也是-1, 对应kNew
const int kAdded = 1;
const int kDeleted = 2;

// ①epoll_create
// vector<epoll_event默认长度是16>
EpollPoller::EpollPoller(EventLoop *loop)
    : Poller()
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events(kIntEventListSize) {
    if(epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
} 
EpollPoller::~EpollPoller() {
    ::close(epollfd_);

}

// 虚函数重写 // ③epoll_wait
Timestamp EpollPoller::poll(int timeousMs, ChannelList *activeChannels) {
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    // &*events_.begin() vector<events>底层的数组的起始地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),static_cast<int>(events_.size()), timeousMs);
    int saveErrno = errno; // poll是不同线程的, errno是全局的

    Timestamp now(Timestamp::now());

    if(numEvents > 0) {
        // 处理吧
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()) {
            // 扩容
            events_.resize(events_.size() * 2);
        }
    }else if(numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }else {
        // <0 发生错误
        if(saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() err!");

        }
        // 外部的中断无需处理
    }


    
}
// ②epoll_ctl add/mod
// channel::update --> eventloop::update -->poller::update
void EpollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    if(index  == kNew || index == kDeleted) {
        // a new one
        int fd = channel->fd();
        if(index == kNew) {
            channels[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }else {
        // an old one
        int fd = channel->fd();
        if(channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }else {
            update(EPOLL_CTL_MOD, channel);
        }
    }


}
// ②epoll_ctl del
void EpollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    int index = channel->index();

    channels_.erase(fd); // 从poller的map中删掉

    if(index == kAdded) { // 从epoll里删掉
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的链接
void EpollPoller::fillActiveChannels(int numEvents, ChannelLists *activeChannels) {
    for(int i=0;i<numEvents;++i) {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // eventloop 拿到poller返回的所有发生事件的channel了
    }
}

// epoll_ctl 
void EpollPoller::update(int operation, Channel *channel) {
    epoll_events event;
    bzero(&events, sizeof event);
    events.events = channel->events();
    events.data.ptr = channel;
    int fd = channel->fd();
    events.data.fd = fd;

    if(::epoll_ctl(epollfd_, operation, fd, &events) < 0) {
        if(operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error = %d \n", errno);
        }else {
            LOG_FATAL("epoll_ctl add/mod error = %d \n", errno);
        }
    }

}
