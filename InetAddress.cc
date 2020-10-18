#include "InetAddress.h"
#include <strings.h>
#include <string.h>
InetAddress::InetAddress(uint16_t port, std::string ip) {
    // 根据port ip 初始化addrin
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    adrr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
    // addr_ 转成本地字节序
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}
std::string InetAddress::toIpPort() const {
    //ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf+end, ":%u", port);
    return buf;
}
uint16_t InetAddress:toPort() const {
    return ntohs(addr_.sin_port);
}


#include <iostream>
int main() {
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
    return 0;
}