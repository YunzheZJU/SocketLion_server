#include <Winsock2.h>
#include <iostream>

using namespace std;

int main() {
    cout << "Start." << endl;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData{};
    int err;

    // Load winsocket dll
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        cout << "Error occurred in initialization: " << WSAGetLastError() << endl;
        return 1;
    } else {
        // Check the lowest and highest byte of the version in HEX
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            cout << "Could not find a usable version of Winsock.dll." << endl;
            WSACleanup();
            return 1;
        } else {
            cout << "The Winsock 2.2 dll was found successfully." << endl;
        }
    }

    // Create socket based on TCP
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


    cout << "Bye." << endl;
    WSACleanup();
    return 0;
}