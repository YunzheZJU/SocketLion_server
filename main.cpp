#include "main.h"

vector<ClientInfo> clientInfo;
vector<thread> threads;
list<int> availableSlots;
atomic_int count = {0};
mutex g_mtx;
int total = 0;

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        ClientInfo newInfo{};
//        SOCKET socketClient;
        newInfo.socket = accept(socketThisServer, (SOCKADDR *) &newInfo.clientAddress, &clientAddressLength);
        if (newInfo.socket == INVALID_SOCKET) {
            cerr << "Invalid socket." << endl;
            continue;
        }
        clog << "New connection: " << inet_ntoa(newInfo.clientAddress.sin_addr) << endl;
        newInfo.order = total;
        g_mtx.lock();
        if (!availableSlots.empty()) {
            int slot = availableSlots.front();
            availableSlots.pop_front();
            clientInfo[slot] = newInfo;
            threads.emplace(threads.begin() + slot, communicate, slot);
        } else {
            clientInfo.push_back(newInfo);
            threads.emplace_back(communicate, total);
        }
        g_mtx.unlock();
        count++;
        total++;
        cout << "Number of threads: " << count << endl;
        cout << "Length of clientInfo: " << clientInfo.size() << endl;
        cout << "Length of availableSlots: " << availableSlots.size() << endl;
    }
#pragma clang diagnostic pop
}

void communicate(int slot) {
    while (true) {
        char request[256];
        int requestLength = recv(clientInfo[slot].socket, request, 256, 0);
        if (requestLength > 0) {
            request[requestLength] = '\0';
            clog << "Data received: " << request << endl;

            SYSTEMTIME start{};
            GetLocalTime(&start);
            string time = to_string(start.wYear) + "/"
                          + to_string(start.wMonth) + "/"
                          + to_string(start.wDay) + " "
                          + to_string(start.wHour) + ":"
                          + AlignTime(to_string(start.wMinute)) + ":"
                          + AlignTime(to_string(start.wSecond));
            string response = "200\r\nNumber: " + to_string(clientInfo[slot].order) + "\r\nIP: " + string(inet_ntoa(clientInfo[slot].clientAddress.sin_addr)) + "\r\nPort: " +
                              to_string(clientInfo[slot].clientAddress.sin_port) + "\r\nTime: " + time +
                              "\r\nServer: SocketLion\r\n\r\nYunzhe";
            send(clientInfo[slot].socket, response.data(), response.length(), 0);
        } else {
            if (requestLength == 0) {
                clog << "Connection is closed." << endl;
            } else {
                cerr << "Error occurred in receiving: " << WSAGetLastError() << "." << endl;
            }
            closesocket(clientInfo[slot].socket);
            count--;
            g_mtx.lock();
            availableSlots.push_back(slot);
            g_mtx.unlock();
            cout << "Number of threads: " << count << endl;
            cout << "Length of clientInfo: " << clientInfo.size() << endl;
            cout << "Length of availableSlots: " << availableSlots.size() << endl;
            break;
        }
    }
}

const string AlignTime(const string &num) {
    if (num.length() == 1) {
        return "0" + num;
    }
    else {
        return num;
    }
}