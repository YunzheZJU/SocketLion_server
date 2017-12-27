#include "main.h"

vector<ClientInfo> clientInfo;
set<int> availableSlots;
//list<Message> messageQueue;
atomic_int count = {0};
mutex mutexAvailableSlots;
mutex mutexClientInfo;
//mutex mutexMessageQueue;
int total = 0;
bool stopServer = false;

int main() {
    vector<thread> threads;
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
            // Clear all threads
            vector<thread>::iterator it;
            for (it = threads.begin(); it != threads.end(); it++) {
                if ((*it).joinable()) {
                    (*it).join();
                }
            }
            clog << "All threads are joined." << endl;
            exit(0);
        }
        ClientInfo newInfo{};
        newInfo.socket = accept(socketThisServer, (SOCKADDR *) &clientAddress, &clientAddressLength);
        if (newInfo.socket == INVALID_SOCKET) {
//            cerr << "No valid socket." << endl;
            Sleep(100);
            continue;
        }
        newInfo.address = inet_ntoa(clientAddress.sin_addr);
        newInfo.port = to_string(clientAddress.sin_port);
        newInfo.number = total;
        clog << "New connection: " << newInfo.address << endl;
        mutexAvailableSlots.lock();
        mutexClientInfo.lock();
        if (!availableSlots.empty()) {
            auto it = availableSlots.begin();
            int slot = *it;
            availableSlots.erase(it);
            clientInfo[slot] = newInfo;
            // !important
            if (threads[slot].joinable()) {
                threads[slot].join();
            }
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
    cout << "Thread " << slot << " starts." << endl;
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
            GenerateContent(request, statusCode, content, slot);
            string response = statusCode + "\r\n";
            response.append("Number: " + to_string(clientInfo[slot].number) + "\r\n");
            response.append("Address: " + clientInfo[slot].address + "\r\n");
            response.append("Port: " + clientInfo[slot].port + "\r\n");
            response.append("Time: " + time + "\r\n");
            response.append("Server: SocketLion\r\n\r\n");
            response.append(content);
            send(clientInfo[slot].socket, response.data(), static_cast<int>(response.length()), 0);
            clog << "Response: " << response << endl;
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
            break;
        } else {
            if (stopServer) {
                break;
            }
//            mutexMessageQueue.lock();
//            Message message = messageQueue.front();
//            messageQueue.pop_front();
//            mutexMessageQueue.unlock();
//            CheckMessage(message, slot);
            Sleep(100);
        }
    }
    clog << "Thread " << slot << " exits." << endl;
}

const string AlignTime(const string &num) {
    if (num.length() == 1) {
        return "0" + num;
    } else {
        return num;
    }
}

void GenerateContent(const string &request, string &statusCode, string &content, int slot) {
    string command = request.substr(0, request.find('\r'));
    if (command == "ALOHA") {
        content = "Yunzhe";
        statusCode = "200";
    } else if (command == "LIST") {
        // List
        content.append("List of users online: \n");
        auto sizeOfClientInfo = static_cast<int>(clientInfo.size());
//        cout << "Count: " << count << endl;
//        cout << "sizeOfClientInfo" << sizeOfClientInfo << endl;
        for (int i = 0; i < sizeOfClientInfo; i++) {
            if (availableSlots.find(i) == availableSlots.end()) {
                mutexClientInfo.lock();
                ClientInfo user = clientInfo[i];
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
        // SEND message at once!
        string headerToNumber = "ToNumber";
        string headerToAddress = "ToAddress";
        string separator = "\r\n\r\n";
        int toNumber;
        string2int(toNumber, GetHeader(request, headerToNumber));
        string toAddress = GetHeader(request, headerToAddress);
        string requestContent = request.substr(request.find(separator) + separator.length());
        string fromNumber = to_string(clientInfo[slot].number);
        auto sizeOfClientInfo = static_cast<int>(clientInfo.size());
        for (int i = 0; i < sizeOfClientInfo; i++) {
            if (availableSlots.find(i) == availableSlots.end()) {
                if (clientInfo[slot].number == toNumber && clientInfo[slot].address == toAddress) {
                    // Send the message
                    string messageRequest = "SEND\r\n";
                    messageRequest.append("FromNumber: " + fromNumber + "\r\n\r\n");
                    messageRequest.append(requestContent);
                    // Implicit conversion from const char* to string
                    content = Request(clientInfo[slot].socket, messageRequest.data());
                    statusCode = "200";
                    break;
                }
            }
            if (i == sizeOfClientInfo - 1) {
                statusCode = "404";
            }
        }
//        Message message = {toNumber, toAddress, fromNumber, 0, requestContent};
//        mutexMessageQueue.lock();
//        messageQueue.emplace_back(message);
//        mutexMessageQueue.unlock();
    } else {
        statusCode = "400";
    }
}

string GetHeader(const string &request, const string &header) {
    string stringToFind = header + ": ";
    string temp = request.substr(request.find(stringToFind) + stringToFind.length());
    return temp.substr(0, temp.find('\r'));
}

//void CheckMessage(Message &message, int slot) {
//    if (message.toNumber == clientInfo[slot].number) {
//        if (message.toAddress == clientInfo[slot].address) {
//            string messageContent = message.message;
//            clog << "A message from user [" << message.fromNumber << "] is sending to user [" << clientInfo[slot].number
//                 << "]...";
//            send(clientInfo[slot].socket, messageContent.data(), static_cast<int>(messageContent.length()), 0);
//            clog << "OK" << endl;
//        } else {
//            // The target user has gone
//            clog << "A message from user [" << message.fromNumber << "] is dropped." << endl;
//            messageQueue.pop_front();
//        }
//    }
//}

const char* Request(SOCKET socketClient, const char request[]) {
    clog << "Sending..." << endl;
    send(socketClient, request, static_cast<int>(strlen(request)), 0);
    clog << "Sending...OK" << endl;

    char response[256];
    int retryCount = 0;
    while (true) {
        if (retryCount == 10) {
            break;
        }
        clog << "Receiving..." << endl;
        int responseLength = recv(socketClient, response, 256, 0);
        if (responseLength > 0) {
            clog << "Receiving...OK" << endl;
            return "200";
        } else if (responseLength == 0) {
            clog << "Target client has closed the connection." << endl;
            return "502";
        } else {
            retryCount++;
            clog << "There is no reply from the client. Retry: " << retryCount << endl;
            Sleep(200);
        }
    }
    clog << "Receiving...failed." << endl;
    return "500";
}

void string2int(int &int_temp, const string &string_temp) {
    stringstream stream(string_temp);
    stream >> int_temp;
}