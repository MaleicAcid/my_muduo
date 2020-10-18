#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
// 一个全局变量, 防止一个线程创建多个eventloop
// __thread是为了每个线程各有一个
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000; // 默认poller超时时间

// notify唤醒subReactor
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_)) {
    
    LOG_DEBUG("eventloop created %p in thread %d \n", this, threadId_);

    if(t_loopInThisThread) {
        // 这个线程已经有一个loop了
        LOG_FATAL("another eventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);

    }else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));


    wakeupChannel_->enableReading();

}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;

}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_) {
        activeChannels_.clear();

        // 两种fd, clientfd wakeupfd(eventfd)
        pollReturnTime_ = poller->poll(kPollTimeMs, &activeChannels_);
        for(Channel *channel : activeChannels_) {
            channel->handleEvent(pollReturnTime_);
        }

        // eventloop事件循环需要执行的回调
        /*
            IO线程 mainLoop accept channel(fd) ==> subLoop
            唤醒后的回调
            mainLoop 事先注册一个cb放在PendingFunctors中
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
}

void EventLoop::quit() {
    quit_ = true;
    if(!isInLoopThread) {
        wakeup(); // 是在其他线程中调用了quit_, subLoop中调用mainLoop::quit

    }
}


// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread()){
        cb();
    }else {
        // 非当前loop线程, 唤醒loop所在线程
        queueInLoop(cb);
    }
}
// cb放入队列中, 唤醒loop所在线程再执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // emplace_back直接构造 push_back是拷贝构造
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的, 当前正在执行回调, 又有新回调来了
    
    if(!isInLoopThread || callingPendingFunctors_) {
        wakeup(); // 唤醒loop所在线程
    }
}


void EventLoop::handleRead() {
    uint64_t one = 1;
    size_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        // 其实读几个字节不重要, 关键是poller被唤醒
        LOG_ERROR("reads %d bytes instead of 8 bytes");
    }
}

void EventLoop::doPendingFunctors() {
    callingPendingFunctors_ = true;
    std::vector<Functor> functors(n, nullptr);

    // 减少 mainLoop 阻塞在新连接下发
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

// 向wakeupFd_写一个 1
void EventLoop::wakeup() {
    uint64_t one  = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}
// channel 向poller问
// 通过父组件eventLoop => Poller的方法
void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

