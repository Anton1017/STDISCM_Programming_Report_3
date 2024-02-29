#include <chrono>
#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
int main() {

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,  2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
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
    serverAddress.sin_addr.S_un.S_addr = inet_addr("172.24.198.250"); // IP address

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    //Send identifier as client
    std::string client = "client";
    send(sock, client.c_str(), client.size(),  0);

    //User input
    std::string temp;
    char buffer[1024] = {0};
    while (std::getline(std::cin, temp))
    {
        if (temp != "Exit") {
            std::cout << "Enter start point: ";
            std::string startPoint;
            std::getline(std::cin, startPoint);

            std::cout << "Enter end point: ";
            std::string endPoint;
            std::getline(std::cin, endPoint);

            std::cout << "Enter number of threads: ";
            std::string numThreads;
            std::getline(std::cin, numThreads);
            std::string message = startPoint + "," + endPoint + "," + numThreads;

            //start timer
            auto start = std::chrono::steady_clock::now();
            send(sock, message.c_str(), message.size(),  0);

            // Receive the size of the primes vector
            int primesSize;
            recv(sock, reinterpret_cast<char*>(&primesSize), sizeof(primesSize), 0);

            // // Receive each element of the primes vector
            // std::vector<int> receivedPrimes(primesSize);
            // for (size_t i = 0; i < primesSize; ++i) {
            //     recv(sock, reinterpret_cast<char*>(&receivedPrimes[i]), sizeof(receivedPrimes[i]), 0);
            // }
            // // Print primes
            // for (int i = 0; i < receivedPrimes.size(); i++)
            // {
            //     std::cout << receivedPrimes[i] <<  std::endl;
            // }
            std::cout << primesSize << " primes were found." << std::endl;

            //end timer
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            std::cout << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl;

        }
        else {
            send(sock, temp.c_str(), temp.size(),  0);
            break;
        }

    }
    recv(sock, buffer,  1024,  0);
    std::cout << "Server: " << buffer << std::endl;

    closesocket(sock);
    WSACleanup();

    return 0;
}
