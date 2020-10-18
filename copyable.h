#pragma once

/*
ncopyable 被继承以后 派生类对象可以正常地构造和析构
进行拷贝构造和赋值
*/
class copyable {
public:
    copyable(const copyable&) = delete;
    void operator=(const copyable&) = delete;

protected:
    copyable() = default;
    ~copyable() = default;
};