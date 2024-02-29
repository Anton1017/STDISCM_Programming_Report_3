#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>

const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";

bool check_prime(const int &n);

void find_primes_range(int start, int end, int limit, std::vector<int> &primes, std::mutex &primes_mutex);

void mutualExclusion(int current_num, std::vector<int> &primes, std::mutex &primes_mutex);

int main() {
    std::vector <int> primes;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,  2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    SOCKET slaveSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slaveSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in slaveAddress;
    slaveAddress.sin_family = AF_INET;
    slaveAddress.sin_port = htons(PORT); // Port number
    slaveAddress.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS); // IP address

    if (connect(slaveSock, (SOCKADDR*)&slaveAddress, sizeof(slaveAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(slaveSock);
        WSACleanup();
        return 1;
    }

    // Receive message from master
    char buffer[1024] = {0};
    recv(slaveSock, buffer,  1024,  0);
    std::cout << "Server: " << buffer << std::endl;

    // Parse the received message to get start, end, and numThreads
    int start, end, numThreads;
    sscanf(buffer, "%d,%d,%d", &start, &end, &numThreads);

    // Perform prime checking
    std::mutex primes_mutex;
    find_primes_range(start, end, end, primes, primes_mutex);

    // Serialize and send the size of the primes vector
    int primesSize = primes.size();
    send(slaveSock, reinterpret_cast<const char*>(&primesSize), sizeof(primesSize), 0);
    
    // Serialize and send each element of the primes vector
    for (int prime : primes) {
        send(slaveSock, reinterpret_cast<const char*>(&prime), sizeof(prime), 0);
    }

    closesocket(slaveSock);
    WSACleanup();

    return 0;
}

void find_primes_range(int start, int end, int limit, std::vector<int> &primes, std::mutex &primes_mutex) {
    for (int current_num = start; current_num <= end && current_num <= limit; current_num++) {
        if (check_prime(current_num)) {
            mutualExclusion(current_num, primes, primes_mutex);
        }
    }
}

void mutualExclusion(int current_num, std::vector<int> &primes, std::mutex &primes_mutex) {
    std::lock_guard<std::mutex> lock(primes_mutex);
    primes.push_back(current_num);
}

bool check_prime(const int &n) {
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}
