#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <future>

const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "172.24.198.250";

bool check_prime(const int &n);

void find_primes_range(int start, int end, int limit, std::vector<int> &primes, std::mutex &primes_mutex);

void mutualExclusion(int current_num, std::vector<int> &primes, std::mutex &primes_mutex);

int main() {
    std::vector <int> primes;
    char buffer[1024] = {0};
    int start, end, numThreads;

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

    //Send identifier as client
    std::string slave = "slave";
    send(slaveSock, slave.c_str(), slave.size(),  0);

    // Receive message from master
    recv(slaveSock, buffer,  1024,  0);
    std::cout << "Server: " << buffer << std::endl;

    // Parse the received message to get start, end, and numThreads
    int start, end, numThreads;
    sscanf(buffer, "%d,%d,%d", &start, &end, &numThreads);

    // Perform prime checking
<<<<<<< HEAD
    std::mutex primes_mutex;
    find_primes_range(start, end, end, primes, primes_mutex);

    // Serialize and send the size of the primes vector
    int primesSize = primes.size();
    send(slaveSock, reinterpret_cast<const char*>(&primesSize), sizeof(primesSize), 0);
    // Serialize and send each element of the primes vector
    for (int prime : primes) {
        send(slaveSock, reinterpret_cast<const char*>(&prime), sizeof(prime), 0);
    }
=======
    // Handle client message
    while (true) {
        memset(buffer,  0, sizeof(buffer));
        int valread = recv(slaveSock, buffer, sizeof(buffer) -  1,  0);
        if (valread ==  0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        } else if (valread <  0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            closesocket(slaveSock);
            WSACleanup();
            break;
        }
        
        std::cout << "Message from client: " << buffer << std::endl;

        // Send to slave process

        // Check for termination message
        if (std::string(buffer) == "Exit") {
            std::cout << "Client sent termination message" << std::endl;
            break;
        }else{
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
            int range = (end - start + 1) / numThreads;
            int remainder = (end - start + 1) % numThreads;
            int end_thread = start + range;

            //Create threads
            //std::thread threads[numThreads];
            std::vector<std::future<void>> futures;

            // Create mutex for mutual exclusion
            std::mutex primes_mutex;

            for (int i = 0; i < numThreads; i++) {
              if(remainder > 0){
                    end_thread++;
                    remainder--;
                }
                //threads[i] = std::thread(find_primes_range, start, end_thread, end ,std::ref(primes), std::ref(primes_mutex));
                futures.push_back(std::async(std::launch::async, find_primes_range, start, end_thread, end, std::ref(primes), std::ref(primes_mutex)));
                start = end_thread + 1;
                end_thread = start + range;
            }

            for(auto &f : futures) {
                f.get();
            }

            // Join threads
            // for (int i = 0; i < numThreads; i++) {
            //     threads[i].join();
            // }

            // Serialize and send the size of the primes vector
            int primesSize = primes.size();
            send(slaveSock, reinterpret_cast<const char*>(&primesSize), sizeof(primesSize), 0);
            // Serialize and send each element of the primes vector
            // for (int prime : primes) {
            //     send(slaveSock, reinterpret_cast<const char*>(&prime), sizeof(prime), 0);
            // }
            //Clear the array
            primes.clear();

            /*
            std::cout << "Start: " << start << std::endl;
            std::cout << "End: " << end << std::endl;
            std::cout << "Num Threads: " << numThreads << std::endl;
            */

        }
    }


>>>>>>> slave_branch

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
<<<<<<< HEAD
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
=======
  if (n < 2) return false;
  for (int i = 2; i * i <= n; i++) {
    if (n % i == 0) {
      return false;
>>>>>>> slave_branch
    }
    return true;
}
