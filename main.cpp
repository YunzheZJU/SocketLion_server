#include <Winsock2.h>
#include <iostream>

int main() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 1, 1 );

    err = WSAStartup( wVersionRequested, &wsaData );//该函数的功能是加载一个Winsocket库版本
    if ( err != 0 ) {
        return 1;
    }
    std::cout << "Hello, World!" << std::endl;
    return 0;
}