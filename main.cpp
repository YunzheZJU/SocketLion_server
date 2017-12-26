#include "main.h"

int main() {
    cout << "Server Start." << endl;
    WORD version = MAKEWORD(2, 2);
    WSADATA data{};

    // Load winsocket dll
    clog << "Loading..." << endl;
    if (WSAStartup(version, &data) == SOCKET_ERROR) {
        cerr << "Error occurred in initialization: " << WSAGetLastError() << "." << endl;
        return -1;
    } else {
        // Check the lowest and highest byte of the version in HEX
        if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2) {
            cerr << "Could not find a usable version of Winsock.dll." << endl;
            WSACleanup();
            return -1;
        }
    }
    clog << "Loading...OK" << endl;

    // Create socket based on TCP
    clog << "Creating..." << endl;
    SOCKET socketThisServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketThisServer == INVALID_SOCKET) {
        cerr << "Error occurred in creating socket: " << WSAGetLastError() << "." << endl;
        WSACleanup();
        return -1;
    }
    clog << "Creating...OK" << endl;

    // Bind the socket
    clog << "Binding..." << endl;
    sockaddr_in addressThisServer{};
    addressThisServer.sin_family = AF_INET;
    addressThisServer.sin_port = htons(SERVER_PORT);
    addressThisServer.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(socketThisServer, (LPSOCKADDR) &addressThisServer, sizeof(addressThisServer)) == SOCKET_ERROR) {
        cerr << "Error occurred in binding socket: " << WSAGetLastError() << "." << endl;
        WSACleanup();
        return -1;
    }
    clog << "Binding...OK" << endl;

    // Listening
    clog << "Listening..." << endl;
    if (listen(socketThisServer, 2) == SOCKET_ERROR) {
        cerr << "Error occurred in listening: " << WSAGetLastError() << "." << endl;
        WSACleanup();
        return -1;
    }
    clog << "Listening...OK" << endl;

    // Preparing for the connection
    sockaddr_in clientAddress{};
    int clientAddressLength = sizeof(clientAddress);
    SOCKET socketClient;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        socketClient = accept(socketThisServer, (SOCKADDR *) &clientAddress, &clientAddressLength);
        if (socketClient == INVALID_SOCKET) {
            cerr << "Invalid socket." << endl;
            continue;
        }
        clog << "New connection: " << inet_ntoa(clientAddress.sin_addr) << endl;

        while (true) {
            char request[256];
            int requestLength = recv(socketClient, request, 256, 0);
            if (requestLength > 0) {
                request[requestLength] = '\0';
                clog << "Data received: " << request << endl;

                SYSTEMTIME start{};
                GetLocalTime(&start);
                string time = to_string(start.wHour) + ":" + to_string(start.wMinute) + ":" + to_string(start.wSecond) + " "
                              + to_string(start.wDay) + " " + to_string(start.wMonth) + " " + to_string(start.wYear);
                string response = "200\r\nNumber: 1\r\nIP: " + string(inet_ntoa(clientAddress.sin_addr)) +"\r\nPort: " + to_string(clientAddress.sin_port) + "\r\nTime: " + time + "\r\nServer: SocketLion\r\n\r\nYunzhe";
                send(socketClient, response.data(), response.length(), 0);
            } else {
                if (requestLength == 0) {
                    clog << "Connection is closed." << endl;
                }
                else {
                    cerr << "Error occurred in receiving: " << WSAGetLastError() << "." << endl;
                }
                closesocket(socketClient);
                break;
            }
        }
    }
#pragma clang diagnostic pop
}