#include <Winsock2.h>
#include <iostream>

using namespace std;

int main() {
    cout << "Start." << endl;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData{};

    // Load winsocket dll
    cout << "Loading..." << endl;
    if (WSAStartup(wVersionRequested, &wsaData) == SOCKET_ERROR) {
        cout << "Error occurred in initialization." << endl;
        return -1;
    } else {
        // Check the lowest and highest byte of the version in HEX
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            cout << "Could not find a usable version of Winsock.dll." << endl;
            WSACleanup();
            return -1;
        } else {
            cout << "The Winsock 2.2 dll was found successfully." << endl;
        }
    }
    cout << "Loading...OK" << endl;

    // Create socket based on TCP
    cout << "Creating..." << endl;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        cout << "Error occurred in creating socket." << endl;
        WSACleanup();
        return -1;
    }
    cout << "Creating...OK" << endl;

    // Bind the socket
    cout << "Binding..." << endl;
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(5555);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(s, (LPSOCKADDR) &sin, sizeof(sin)) == SOCKET_ERROR) {
        cout << "Error occurred in binding socket." << endl;
        WSACleanup();
        return -1;
    }
    cout << "Binding...OK" << endl;

    // Listening
    cout << "Listening..." << endl;
    if (listen(s, 2) == SOCKET_ERROR) {
        cout << "Error occurred in listening." << endl;
        WSACleanup();
        return -1;
    }
    cout << "Listening...OK" << endl;

    // Preparing for the connection
    sockaddr_in remoteAddress;
    int nAddressLength = sizeof(remoteAddress);
    SOCKET client;
    char szText[] = "Yunzhe\n";

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true)
    {
        client = accept(s, (SOCKADDR *) &remoteAddress, &nAddressLength);
        if (client == INVALID_SOCKET) {
            cout << "Invalid socket." << endl;
            continue;
        }

        cout << "New connection: " << inet_ntoa(remoteAddress.sin_addr) << endl;

        send(client, szText, strlen(szText), 0);

        closesocket(client);
    }
#pragma clang diagnostic pop

    cout << "Bye." << endl;
    closesocket(s);
    WSACleanup();
    return 0;
}