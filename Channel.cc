#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUUT;

Channel::Channel(EventLoop *loop, int fd)
     : loop_(loop),
       fd_(fd),
       events_(0),
       revents_(0),
       index_(-1),
       tied_(false) {

}
Channel::~Channel() {
    // 判断是在thread里
}

// fd得到poller通知后处理事件,调用回调函数
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;

    if(tied_) {
        guard = tie_.lock(); // 弱回调
        if(gurad) {
            handleEventWithGuard(receiveTime);
        }
    }else {
        handleEventWithGuard(receiveTime);
    }
}

// 根据发生的具体事件调用回调
void Channel::handleEventWithGuard(Timestamp receiveTime) {

    LOG_INFO("channel handelEvent revents:%d\n", revents_);
    
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if(closeCallback_) {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ &(EPOLLIN | EPOLLPRI)) {
        if(readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if(revents_ &(EPOLLOUT)) {
        if(writeCallback_) {
            writeCallback_(receiveTime);
        }
    }
}

// tie()方法在
// 防止channel没手动remove掉, channel还在执行回调
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示的fd的事件后, 
// update负责在poller中负责更新epoll_ctl
void Channel::update() {
    // todo: add code 
    loop_->updateChannel(this);
}
void Channel::remove() {
    loop_->removeChannel(this);
}