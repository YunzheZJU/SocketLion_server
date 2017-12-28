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
#include <list>
#include <mutex>
#include <windows.h>
#include <sstream>

using namespace std;

#define SERVER_PORT 1551

struct ClientInfo {
    SOCKET socket;
    string address;
    string port;
    int number;
};

void interrupt();

void communicate(int slot);

const string AlignTime(const string &num);

void GenerateContent(const string &request, string &statusCode, string &content, int slot);

string GetValue(const string &request, const string &keyword);

string GetTime();

void string2int(int &int_temp, const string &string_temp);

#endif //SOCKETLION_SERVER_MAIN_H
