#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>
static std::atomic_int numCreated_(0);
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name) {
    
    setDefaultName();
}
Thread::~Thread() {
    if(started_ && !joined_) {
        thread_->detach(); // thread提供的设置分离线程方法
    }  
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启子线程, 专门执行该线程函数
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);

        func_(); 
    }));

    // 主线程继续执行
    // 这里必须等待获取上面新创建线程的tid, 使用信号量
    sem_wait(&sem); // 主线程阻塞直到子线程创建成功
}
void Thread::join() {
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName() {
    int num = ++numCreated_; // 已经是atmoic类型了
    if(name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d" , num);
        name_ = buf;
    }
}