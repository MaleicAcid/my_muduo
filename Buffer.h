#pragma once
// 8字节包头 + 包体
// readIndex writeIndex
#include <vector>
#include <string>
class Buffer
{
public:
    Buffer(/* args */);
    ~Buffer();
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend+initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend) {

     }

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    // 可写大小
    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }
    // 可读取的长度
    size_t prependableBytes() const {
        return readerIndex_;
    }

    // 返回缓冲区可读数据的起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    // buffer内容序列化成string
    void retrieve(size_t len) {
        if(len < readableBytes()) {
            readerIndex_ += len;
        }else {
            retrieveAll();
        }
    }
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len);
        retrieve(len); // 缓冲区指针移动
        return result;
    }

    // len大于可写缓冲区
    // buffer.size() - writerIndex_ < len
    void ensureWritebaleBytes(size_t len) {
        if(writableBytes() < len) {
            makeSpace(len);
        }
    }

private:
    // 数据添加到writeable缓冲区中
    void append(const char *data, size_t len) {
        ensureWritebaleBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }
    char* beginWrite() {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const {
        return begin() + writerIndex_;
    }
    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);

    // 从fd上发送数据
    ssize_t writeFd(int fd, int* saveErrno);

    char* begin() {
        // it.operator*() 获取首元素
        // & 获取数组首元素地址也就是数组起始地址
        return  &*buffer_.begin();
    }
    const char* begin() const {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len) {
        /*
            kCheap | reader | writer
            kCheap |       len
        */ 
        if(writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        }else { // 空闲的够 挪一挪
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin()+kCheapPrepend); // 往前挪
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;

        }
    }

    // 底层自动扩容方便但系统api要用裸指针
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};