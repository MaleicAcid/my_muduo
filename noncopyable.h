#pragma once

/*
noncopyable 被继承以后 派生类对象可以正常地构造和析构
无法进行拷贝构造和赋值
*/
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};