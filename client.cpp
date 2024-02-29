#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
std::vector<int> deserializePrimes(const std::string& serializedPrimes);
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
    serverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // IP address

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

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

            // Start timer
            auto start_time{std::chrono::steady_clock::now()};
            send(sock, message.c_str(), message.size(),  0);

            // Receive the size of the primes vector
            int primesSize;
            recv(sock, reinterpret_cast<char*>(&primesSize), sizeof(primesSize), 0);

            // Receive each element of the primes vector
            int bytesReceived = recv(sock, buffer, 1024 - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0'; // Ensure null-termination
                std::string serializedPrimes(buffer);
                std::vector<int> primes = deserializePrimes(serializedPrimes);
                // Now primes contains the list of primes
                for (int i = 0; i < primes.size(); i++)
                {
                    std::cout << primes[i] <<  std::endl;
                }
                std::cout << primes.size() << " primes were found." << std::endl;
            }
            //Print primes 

              // End timer
            auto end_time{std::chrono::steady_clock::now()};

            std::chrono::duration<double> elapsed{end_time - start_time};
            std::cout << "Time: " << elapsed.count() << "s\n";

        }
        else {
            send(sock, temp.c_str(), temp.size(),  0);
            break;
        }

    }
    // recv(sock, buffer,  1024,  0);
    // std::cout << "Server: " << buffer << std::endl;

    closesocket(sock);
    WSACleanup();

    return 0;
}

std::vector<int> deserializePrimes(const std::string& serializedPrimes) {
    std::vector<int> primes;
    std::stringstream ss(serializedPrimes);
    std::string item;
    while (std::getline(ss, item, ',')) {
        primes.push_back(std::stoi(item));
    }
    return primes;
}
