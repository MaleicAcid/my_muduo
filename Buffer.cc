#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/*
    LT 模式下的fd上读取数据
    没读完会不断上报
    Buffer缓冲器区是有大小的，但从fd上读的时候，不知道tcp数据最终大小
*/
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536] = {0}; // 栈上的内存空间 64k
 
    // readv(), writev() 系统调用 可以自动填充多个不同的缓冲区
    // struct iovec 包含iov_base iov_len
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // 先填充vec[0], 然后再填充extrabuf
    // 根据extrabuf再分配Buffer

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0 ) {
        *saveErrno = errno;
    }else(n <= writable) { // Buffer的可写缓冲区已经够
        writerIndex_ += n;
    }else { // extrabuf里也写入了数据
        // Buffer扩容再添加
        writerIndex_ = buffer_.size();

        // 写n-writable大小的数据
        append(extrabuf, n-writable);
    }
    return n;

}
ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    size_t n = ::write(fd, peed(), readableBytes());
    if(n < 0) {
        *saveErrno = errno;
    }
    // >=0 就都算正确
    return n;
}