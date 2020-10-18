#include "Poller.h"
#include "EpollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop) {
    if(::getenv("MUDUO_USE_POLL")) {
        // return nullptr;
        return new EpollPoller(loop);
    }else {
        return new EpollPoller(loop);
    }
    
}