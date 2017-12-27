//
// Created by Yunzhe on 2017/12/25.
//

#ifndef SOCKETLION_SERVER_MAIN_H
#define SOCKETLION_SERVER_MAIN_H

#include <Winsock2.h>
#include <thread>
#include <iostream>
#include <atomic>
#include <vector>
#include <set>
#include <mutex>

using namespace std;

#define SERVER_PORT 1551

struct ClientInfo {
    SOCKET socket;
    sockaddr_in clientAddress;
    int order;
};

void communicate(int slot);

const string AlignTime(const string &num);

void GenerateContent(const string &request, string &statusCode, string &content);

#endif //SOCKETLION_SERVER_MAIN_H
