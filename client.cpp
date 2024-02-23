#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

int main() {

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,  2), &wsaData);
    if (result !=  0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return  1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(6900); // Port number
    serverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP address

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }


    const char *message = "Hello, Server!";
    send(sock, message, strlen(message),   0);

    char buffer[1024] = {0};
    recv(sock, buffer,   1024,   0);
    std::cout << "Server: " << buffer << std::endl;


    closesocket(sock);
    WSACleanup();

    return 0;
}
