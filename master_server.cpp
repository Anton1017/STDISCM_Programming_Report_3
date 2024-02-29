#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <stdlib.h>
#include <mutex>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";
const char* SLAVE_SERVER_ADDRESS = "127.0.0.1";
const int SLAVE_SERVER_PORT = 6900;

bool check_prime(const int &n);

void find_primes_range(int start, int end, int limit, std::vector<int> &primes, mutex &primes_mutex);

void mutualExclusion(int current_num, vector<int> &primes, mutex &primes_mutex);

SOCKET createSlaveSocket(const char* slaveAddress, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET slaveSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slaveSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    sockaddr_in slaveAddr;
    slaveAddr.sin_family = AF_INET;
    slaveAddr.sin_port = htons(port);
    slaveAddr.sin_addr.S_un.S_addr = inet_addr(slaveAddress);

    if (connect(slaveSock, (SOCKADDR*)&slaveAddr, sizeof(slaveAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to slave failed: " << WSAGetLastError() << std::endl;
        closesocket(slaveSock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return slaveSock;
}

int main() {
    std::vector <int> primes;
    char buffer[BUFFER_SIZE] = {0};
    int start, end, numThreads;
    
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return -1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket\n";
        WSACleanup();
        return -1;
    }

    // Configure server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, SERVER_ADDRESS, &(serverAddr.sin_addr.S_un.S_addr)) <=  0) {
        std::cerr << "inet_pton failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return   1;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Error listening for connections\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        // Accept a client connection
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        // Handle the connection
        std::cout << "Accepted connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        // Handle client message
        while (true) {
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            std::cout << "Received task from client: " << buffer << std::endl;

            memset(buffer,  0, sizeof(buffer));
            int valread = recv(clientSocket, buffer, sizeof(buffer) -  1,  0);
            if (valread ==  0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            } else if (valread <  0) {
                std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                return -1;
            }
            
            std::cout << "Message from client: " << buffer << std::endl;
            if (std::string(buffer) == "Exit") {
                std::cout << "Client sent termination message" << std::endl;
                break;
            }
            // Split message into start, end, and num threads
            int i = 0;
            std::string temp = "";
            
            char* ptr;
            ptr = strtok(buffer, ",");
            start = std::stoi(ptr);
            ptr = strtok(NULL, ",");
            end = std::stoi(ptr);
            ptr = strtok(NULL, ",");
            numThreads = std::stoi(ptr);

            // Get the range for each thread
            int range = end / numThreads;
            int end_thread = start + range;

            //Create threads
            std::thread threads[numThreads];

            // Create mutex for mutual exclusion
            mutex primes_mutex;

            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Error accepting connection\n";
                continue;
            }

            char buffer[BUFFER_SIZE] = {0};
            recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            for (int i = 0; i < numThreads; i++) {
                threads[i] = std::thread(find_primes_range, start, end_thread, end ,std::ref(primes), std::ref(primes_mutex));
                start = end_thread + 1;
                end_thread = start + range;
            }

            // Join threads
            for (int i = 0; i < numThreads; i++) {
                threads[i].join();
            }

            // Serialize and send the size of the primes vector
            // int primesSize = primes.size();
            // send(clientSocket, reinterpret_cast<const char*>(&primesSize), sizeof(primesSize), 0);
            
            // Serialize and send each element of the primes vector
            // for (int prime : primes) {
            //     send(clientSocket, reinterpret_cast<const char*>(&prime), sizeof(prime), 0);
            // }

            //Clear the array
            primes.clear();

            // Send to slave process
            SOCKET slaveSock = createSlaveSocket(SLAVE_SERVER_ADDRESS, SLAVE_SERVER_PORT);
            if (slaveSock != INVALID_SOCKET) {
                send(slaveSock, buffer, strlen(buffer), 0);
                // Receive results from slave server and send back to client
                char resultBuffer[BUFFER_SIZE] = {0};
                recv(slaveSock, resultBuffer, sizeof(resultBuffer) - 1, 0);
                send(clientSocket, resultBuffer, strlen(resultBuffer), 0);
                closesocket(slaveSock);
            }
        }

        // Send a message to the connected client
        const char* message = "You have been disconnected to the server";
        send(clientSocket, message, strlen(message), 0);

        // Close the client socket
        closesocket(clientSocket);
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
void find_primes_range(int start, int end, int limit, vector<int> &primes, mutex &primes_mutex) {
  for (int current_num = start; current_num <= end && current_num <= limit; current_num++) {
    if (check_prime(current_num)) {
      mutualExclusion(current_num, primes, primes_mutex);
    }
  }
}

void mutualExclusion(int current_num, vector<int> &primes, mutex &primes_mutex) {
  lock_guard<mutex> lock(primes_mutex);
  primes.push_back(current_num);
}

bool check_prime(const int &n) {
  // Special case for 1
  if (n < 2) return false;
  for (int i = 2; i * i <= n; i++) {
    if (n % i == 0) {
      return false;
    }
  }
  return true;
}
