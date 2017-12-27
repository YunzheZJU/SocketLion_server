#include "main.h"

vector<ClientInfo> clientInfo;
vector<thread> threads;
set<int> availableSlots;
atomic_int count = {0};
mutex mutexAvailableSlots;
mutex mutexClientInfo;
int total = 0;
bool stopServer = false;

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

    unsigned long ul = 1;
    int ret = ioctlsocket(socketThisServer, FIONBIO, &ul);  // 设置socketThisServer为非阻塞模式
    if (ret == SOCKET_ERROR) {
        cout << "Error." << endl;
    }

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
    thread threadInterrupt(interrupt);
    cout << "Waiting for connections..." << endl;
    while (true) {
        if (stopServer) {
            // TODO: 清除所有线程
            exit(0);
        }
        ClientInfo newInfo{};
        newInfo.socket = accept(socketThisServer, (SOCKADDR *) &clientAddress, &clientAddressLength);
        if (newInfo.socket == INVALID_SOCKET) {
//            cerr << "Invalid socket." << endl;
            Sleep(100);
            continue;
        }
        newInfo.address = inet_ntoa(clientAddress.sin_addr);
        newInfo.port = clientAddress.sin_port;
        newInfo.number = total;
        clog << "New connection: " << newInfo.address << endl;
        mutexAvailableSlots.lock();
        mutexClientInfo.lock();
        if (!availableSlots.empty()) {
            auto it = availableSlots.begin();
            int slot = *it;
            availableSlots.erase(it);
            clientInfo[slot] = newInfo;
            threads.emplace(threads.begin() + slot, communicate, slot);
        } else {
            clientInfo.push_back(newInfo);
            threads.emplace_back(communicate, total);
        }
        mutexAvailableSlots.unlock();
        mutexClientInfo.unlock();
        count++;
        total++;
        cout << "Number of threads(clients): " << count << endl;
        cout << "Length of clientInfo: " << clientInfo.size() << endl;
        cout << "Number of availableSlots: " << availableSlots.size() << endl;
    }
#pragma clang diagnostic pop
}

void interrupt() {
    getchar();
    stopServer = true;
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
            string statusCode;
            string content;
            GenerateContent(request, statusCode, content);
            string response = statusCode + "\r\n";
            response.append("Number: " + to_string(clientInfo[slot].number) + "\r\n");
            response.append("Address: " + clientInfo[slot].address + "\r\n");
            response.append("Port: " + clientInfo[slot].port + "\r\n");
            response.append("Time: " + time + "\r\n");
            response.append("Server: SocketLion\r\n\r\n");
            response.append(content);
            send(clientInfo[slot].socket, response.data(), static_cast<int>(response.length()), 0);
        } else if (requestLength == 0) {
            clog << "Connection is closed." << endl;
            closesocket(clientInfo[slot].socket);
            count--;
            mutexAvailableSlots.lock();
            availableSlots.insert(slot);
            mutexAvailableSlots.unlock();
            cout << "Number of threads(clients): " << count << endl;
            cout << "Length of clientInfo: " << clientInfo.size() << endl;
            cout << "Number of availableSlots: " << availableSlots.size() << endl;
            // TODO: 移除线程
            break;
        }
    }
}

const string AlignTime(const string &num) {
    if (num.length() == 1) {
        return "0" + num;
    } else {
        return num;
    }
}

void GenerateContent(const string &request, string &statusCode, string &content) {
    string command = request.substr(0, request.find('\r'));
    if (command == "ALOHA") {
        content = "Yunzhe";
        statusCode = "200";
    } else if (command == "LIST") {
        // List
        auto sizeOfClientInfo = static_cast<int>(clientInfo.size());
        cout << "Count: " << count << endl;
        cout << "sizeOfClientInfo" << sizeOfClientInfo << endl;
        for (int slot = 0; slot < sizeOfClientInfo; slot++) {
            if (availableSlots.find(slot) == availableSlots.end()) {
                mutexClientInfo.lock();
                ClientInfo user = clientInfo[slot];
                mutexClientInfo.unlock();
                string userNumber = to_string(user.number);
                string userAddress = user.address;
                string userPort = user.port;
                content.append(userNumber + "\t");
                content.append(userAddress + "\t");
                content.append(userPort + "\n");
            }
        }
        statusCode = "200";
    } else if (command == "SEND") {
        // SEND
    } else {
        statusCode = "400";
    }
}