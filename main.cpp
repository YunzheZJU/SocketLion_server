#include "main.h"

vector<ClientInfo> clientInfo;
vector<string> clientData;
set<int> availableSlots;
atomic_int count = {0};
mutex mutexAvailableSlots;
mutex mutexClientInfo;
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

    thread threadInterrupt(interrupt);
    cout << "Waiting for connections..." << endl;
    while (!stopServer) {
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
            clientData[slot] = "";
            // !important
            if (threads[slot].joinable()) {
                threads[slot].join();
            }
            threads.emplace(threads.begin() + slot, communicate, slot);
        } else {
            clientInfo.push_back(newInfo);
            clientData.emplace_back("");
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
    // Clear all threads
    vector<thread>::iterator it;
    for (it = threads.begin(); it != threads.end(); it++) {
        if ((*it).joinable()) {
            (*it).join();
        }
    }
    clog << "All threads are joined." << endl;
    cout << "Bye." << endl;
    return 0;
}

void interrupt() {
    getchar();
    stopServer = true;
}

void communicate(int slot) {
    cout << "Thread " << slot << " starts." << endl;
    char request[256];
    int retryCount = 0;
    while (!stopServer) {
        // In some cases, many requests will be packed together so it is necessary to buffer and unpacked them by endtag
        int requestLength = recv(clientInfo[slot].socket, request, 256, 0);
        if (requestLength > 0) {
            retryCount = 0;
            request[requestLength] = '\0';
            clog << "Data received: " << request << endl;
            string dataAppend = request;
            clientData[slot] += dataAppend;
            auto endTag = clientData[slot].find("\r\t\n");
            while (endTag != clientData[slot].npos) {
                string time = GetTime();
                string statusCode;
                string content;
                string stringRequest = clientData[slot].substr(0, endTag);
                clientData[slot].erase(0, endTag + 3);
//                cout << "ClientData[" << slot << "]: " << clientData[slot] << endl;
                GenerateContent(stringRequest, statusCode, content, slot);
                if (statusCode == "302") {
                    // Message has been sent to another client in GenerateContent.
                    clog << "Another thread will take over my work!" << endl;
                } else {
                    string response = statusCode + "\r\n";
                    response.append("Number: " + to_string(clientInfo[slot].number) + "\r\n");
                    response.append("Address: " + clientInfo[slot].address + "\r\n");
                    response.append("Port: " + clientInfo[slot].port + "\r\n");
                    response.append("Time: " + time + "\r\n");
                    response.append("Server: SocketLion\r\n");
                    response.append("\r\n");
                    response.append(content);
                    response.append("\r\t\n");
                    send(clientInfo[slot].socket, response.data(), static_cast<int>(response.length()), 0);
                    clog << "Response: " << response << endl;
                }
                endTag = clientData[slot].find("\r\t\n");
            }
        } else if (requestLength == 0 || retryCount > 6000) {
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
            retryCount++;
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
    content.append("Method: " + command + "\n");
    if (command == "ALOHA") {
        content.append("Yunzhe");
        statusCode = "200";
    } else if (command == "TIME") {
        content.append(GetTime());
        statusCode = "200";
    } else if (command == "SERV") {
        content.append("SocketLion");
        statusCode = "200";
    } else if (command == "LIST") {
        // List
        content.append("List of users online: \n");
        auto sizeOfClientInfo = static_cast<int>(clientInfo.size());
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
    } else if (command == "SEND" || command == "REPLY") {
        // Redirect the message
        string keywordToNumber = "ToNumber";
        string keywordToAddress = "ToAddress";
        string keywordToPort = "ToPort";
        string separator = "\r\n\r\n";
        int toNumber;
        string2int(toNumber, GetValue(request, keywordToNumber));
        string toAddress = GetValue(request, keywordToAddress);
        string toPort = GetValue(request, keywordToPort);
        string message = request.substr(request.find(separator) + separator.length());
        string fromNumber = to_string(clientInfo[slot].number);
        string fromAddress = clientInfo[slot].address;
        string fromPort = clientInfo[slot].port;
        auto sizeOfClientInfo = static_cast<int>(clientInfo.size());
        for (int i = 0; i < sizeOfClientInfo; i++) {
            if (availableSlots.find(i) == availableSlots.end()) {
                if (clientInfo[i].number == toNumber
                    && clientInfo[i].address == toAddress
                    && clientInfo[i].port == toPort) {
                    // Target client is found
                    statusCode = (command == "SEND" ? "302" : "200");
                    string time = GetTime();
                    string response = statusCode + "\r\n";
                    response.append("Number: " + to_string(toNumber) + "\r\n");
                    response.append("Address: " + toAddress + "\r\n");
                    response.append("Port: " + toPort + "\r\n");
                    response.append("Time: " + time + "\r\n");
                    response.append("Server: SocketLion\r\n");
                    response.append("FromNumber: " + fromNumber + "\r\n");
                    response.append("FromAddress: " + fromAddress + "\r\n");
                    response.append("FromPort: " + fromPort + "\r\n");
                    response.append("\r\n");
                    response.append("Method: " + command + "\n");
                    response.append(message);
                    response.append("\r\t\n");
                    send(clientInfo[i].socket, response.data(), static_cast<int>(response.length()), 0);
                    statusCode = "302";
                    break;
                }
            }
            if (i == sizeOfClientInfo - 1) {
                statusCode = "404";
            }
        }
    } else {
        statusCode = "400";
    }
}

string GetValue(const string &request, const string &keyword) {
    string stringToFind = keyword + ": ";
    string temp = request.substr(request.find(stringToFind) + stringToFind.length());
    return temp.substr(0, temp.find('\r'));
}

string GetTime() {
    SYSTEMTIME start{};
    GetLocalTime(&start);
    return to_string(start.wYear) + "/"
           + to_string(start.wMonth) + "/"
           + to_string(start.wDay) + " "
           + to_string(start.wHour) + ":"
           + AlignTime(to_string(start.wMinute)) + ":"
           + AlignTime(to_string(start.wSecond));
}

void string2int(int &int_temp, const string &string_temp) {
    stringstream stream(string_temp);
    stream >> int_temp;
}