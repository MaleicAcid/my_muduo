#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
class Channel;
class Poller;

// EventLoop = Channel + Poller
class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    // 开启, 退出循环
    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    // 在当前loop中执行
    void runInLoop(Functor cb);
    // cb放入队列中, 唤醒loop所在线程再执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();

    // eventLoop => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_; // 原子操作的布尔值CAS
    std::atomic_bool quit_; // 标志退出loop循环, 原子操作的布尔值CAS
    std::atomic_bool callingPendingFunctors_; // 标志当前loop当前是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_; // 互斥锁, 用来保护上面容器的线程安全

    const pid_t threadId_; // loop所在线程id

    Timestamp pollReturnTime_; //poller返回发生事件channel的时间点

    std::unique_ptr<Poller> poller_;

    // mainReactor如何把新连接的channel扔给subReactor(轮询算法选择subLoop+eventfd()唤醒)
    // libevent采用socketpair
    // muduo 采用eventfd()系统调用,是线程间的通信机制, 内核空间直接通知用户空间的应用
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;// 管理的所有channel
};