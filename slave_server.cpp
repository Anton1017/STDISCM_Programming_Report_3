#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

int wmain() {
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,  2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup() failed with error: %d\n", iResult);
        return  1;
    }

    // Create a SOCKET for listening for incoming connection requests.
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return  1;
    }

    // The sockaddr_in structure specifies the address family, IP address, and port for the socket that is being bound.
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1"); // Listen on  127.0.0.1
    service.sin_port = htons(27015); // Example port number, change as needed

    iResult = bind(ListenSocket, (SOCKADDR *) &service, sizeof(service));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"bind function failed with error %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return  1;
    }

    // Listen for incoming connection requests on the created socket
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return  1;
    }

    wprintf(L"Listening on socket...\n");

    // Accept and process connections here
    // For demonstration, this example does not include the connection handling loop

    // Cleanup
    iResult = closesocket(ListenSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return  1;
    }

    WSACleanup();
    return  0;
}