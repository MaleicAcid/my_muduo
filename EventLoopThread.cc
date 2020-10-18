#include "EventLoopThread.h"
#include "EventLoop.h"
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this) , name)
    , mutex_()
    , cond_()
    , callback_(cb) {

}
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    thread_.start(); // 开启新线程, 执行EventLoopThread::threadFunc

    EventLoop *loop = nullptr;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while( loop == nullptr ) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    // 返回新线程的loop指针
    return loop;
}


// 在单独的新线程里面执行
void EventLoopThread::threadFunc() {
    // 在EventLoopThread::threadFunc()中创建了一个和线程对应的 eventloop
    // 这就是one loop per thread
    EventLoop loop;

    if(callback_) {
        callback_(&loop); // 针对绑定的这个loop做一些事情
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one(); // 通知主线程我已经初始化好了
    }

    loop.loop();// 子线程开启loop

    // loop退出
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;

}