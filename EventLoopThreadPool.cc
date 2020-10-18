#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
      : baseLoop_(baseLoop)
      , name_(nameArg)
      , started_(false)
      , numThreads_(0)
      , next_(0) {

}
EventLoopThreadPool::~EventLoopThreadPool() {
    // 线程绑定的loop都是栈上的对象, 不用手动delete
}


void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;

    // 整个服务端有多个线程,
    for(int i=0;i<numThreads_;++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); // 创建线程, 绑定eventLoop, 并返回loop地址
    }

    // 整个服务端只有一个线程, 只有mainLoop
    if(numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

// 以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = baseLoop_; // 新链接分配给自己
    if(!loops_.empty()) { // 新链接轮询分配给其他线程
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop; 
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if(loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
}