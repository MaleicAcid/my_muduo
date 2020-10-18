#include "Poller.h"
#include "Channel.h"
Poller::Poller(EventLoop *loop) : ownerLoop_(loop){

}

bool Poller::hasChannel(Channel *channel) const {
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

// 这个静态方法不要写在这里, 这样引入派生类的头文件, 这样不好
// #include "PollPoller.h" ...
// #include "EpollPoller.h"
//static Poller* newDefaultPoller(EventLoop *loop) { ... }