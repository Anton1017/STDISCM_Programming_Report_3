#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <stdlib.h>
#include <mutex>
#include <vector>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "172.24.198.250";

// for locking when adding/removing slave socket from vector
mutex slaveCountMutex;

// for locking currSlaveJobs whenever a slave finishes the job
mutex slaveJobMutex;

// for tracking number of slaves currently computing
int currSlaveJobs = 0;

std::vector<int> primes;

int primesCount = 0;

vector<SOCKET> slaveSockets;

bool check_prime(const int &n);

void find_primes_range(int start, int end, int limit, std::vector<int> &primes, mutex &primes_mutex);

void mutualExclusion(int current_num, vector<int> &primes, mutex &primes_mutex);

void acceptClients(SOCKET serverSocket);

void handleClient(SOCKET clientSocket);

void handleSlave(SOCKET slaveSocket, mutex &slaveCountMutex, mutex &slaveJobMutex);

std::vector<std::pair<int,int>> getJobList(int start, int end, int numWorkers);

int main() {
    std::vector <int> primes;
    char buffer[BUFFER_SIZE] = {0};
    int start, end, numThreads;
    
    // Initialize Winsock
    WSADATA wsaData;
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

    std::cout << "Server IP Address: " << SERVER_ADDRESS << std::endl;

    std::cout << "Server listening on port " << PORT << std::endl;

    string str;

    std::thread acceptClientsThread(acceptClients, serverSocket);
    acceptClientsThread.detach();  // Detach the thread to allow it to run independently

    while(true) {
        std::cin >> str;
        std::cout << str;
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

void acceptClients(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        // Accept a client connection
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        memset(buffer,  0, sizeof(buffer));
        int valread = recv(clientSocket, buffer, sizeof(buffer) -  1,  0);
        if (valread ==  0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        } else if (valread <  0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            break;
        }

        // Handle the connection
        

        if(strcmp(buffer, "client") == 0){
            // Handle the connection in a separate thread
            std::cout << "Accepted client connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();  // Detach the thread to allow it to run independently
        }else if(strcmp(buffer, "slave") == 0){
            std::cout << "Accepted slave connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            std::thread slaveThread(handleSlave, clientSocket, std::ref(slaveCountMutex), std::ref(slaveJobMutex));
            slaveThread.detach();
            unique_lock<mutex> lock(slaveCountMutex);
            slaveSockets.push_back(clientSocket);
            lock.unlock();
            // Send a message to the connected client
            send(clientSocket, SERVER_ADDRESS, strlen(SERVER_ADDRESS), 0);
        }
        

        // Note: Avoid using join() here to prevent blocking the main loop.

        // The loop will continue to accept new connections and handle them in separate threads.
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    int start, end, numThreads;


    // Handle client message
    while (true) {
        memset(buffer,  0, sizeof(buffer));
        int valread = recv(clientSocket, buffer, sizeof(buffer) -  1,  0);
        if (valread ==  0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        } else if (valread <  0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
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

            // Get the job (start and end) for each active slave server + master server
            std::vector<pair<int,int>> jobList = getJobList(start, end, slaveSockets.size() + 1);

            currSlaveJobs = slaveSockets.size();

            // send jobs to slaves
            for(auto slaveSock: slaveSockets){
                pair<int,int> job = jobList.back();
                jobList.pop_back();
                std::string jobStr = to_string(job.first) + "," + to_string(job.second) + "," + to_string(numThreads);
                send(slaveSock, jobStr.c_str(), jobStr.size(),  0);
            }

            // Get the range for each thread
            pair<int,int> job = jobList.back();
            jobList.pop_back();
            int masterStart = job.first;
            int masterEnd = job.second;
            int range = (masterEnd - masterStart + 1) / numThreads;
            int remainder = (masterEnd - masterStart + 1) % numThreads;
            int end_thread = masterStart + range;

            //Create threads
            std::thread threads[numThreads];

            // Vector for temporary storage of master prime computations
            std::vector<int> primesMaster;

            // Create mutex for mutual exclusion
            mutex primes_mutex;

            for (int i = 0; i < numThreads; i++) {
                if(remainder > 0){
                    end_thread++;
                    remainder--;
                }
                threads[i] = std::thread(find_primes_range, masterStart, end_thread, masterEnd, std::ref(primesMaster), std::ref(primes_mutex));
                masterStart = end_thread + 1;
                end_thread = masterStart + range;
            }

            // Join threads
            for (int i = 0; i < numThreads; i++) {
                threads[i].join();
            }

            unique_lock<mutex> lock(slaveJobMutex);
            // primes.insert(std::end(primes), std::begin(primesMaster), std::end(primesMaster));
            primesCount += primes.size();
            
            lock.unlock();

            // wait for slave jobs to finish
            while(currSlaveJobs != 0){
                continue;
            }

            // Serialize and send the size of the primes vector
            int primesSize = primesCount;
            send(clientSocket, reinterpret_cast<const char*>(&primesSize), sizeof(primesSize), 0);
            // Serialize and send each element of the primes vector
            // for (int prime : primes) {
            //     send(clientSocket, reinterpret_cast<const char*>(&prime), sizeof(prime), 0);
            // }
            //Clear the array
            primes.clear();

            primesCount = 0;

            /*
            std::cout << "Start: " << start << std::endl;
            std::cout << "End: " << end << std::endl;
            std::cout << "Num Threads: " << numThreads << std::endl;
            */

        }
    }

    // Send a message to the connected client
    const char* message = "You have been disconnected from the server";
    send(clientSocket, message, strlen(message), 0);

    // Close the client socket
    closesocket(clientSocket);
}

void handleSlave(SOCKET slaveSocket, mutex &slaveCountMutex, mutex &slaveJobMutex){
    while(true){
        // Receive the size of the primes vector
        int primesSize;
        recv(slaveSocket, reinterpret_cast<char*>(&primesSize), sizeof(primesSize), 0);
        // Receive each element of the primes vector
        std::vector<int> receivedPrimes(primesSize);
        // for (int i = 0; i < primesSize; ++i) {
        //     recv(slaveSocket, reinterpret_cast<char*>(&receivedPrimes[i]), sizeof(receivedPrimes[i]), 0);
        // }
        unique_lock<mutex> lock(slaveJobMutex);
        // primes.insert(std::end(primes), std::begin(receivedPrimes), std::end(receivedPrimes));
        primesCount += primesSize;
        currSlaveJobs--;
        lock.unlock();
    }
    
    // decrement number of slaves
    lock_guard<mutex> lock(slaveCountMutex);
    auto it = find(slaveSockets.begin(), slaveSockets.end(), slaveSocket);
    slaveSockets.erase(it);
    closesocket(slaveSocket);
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
  if (n < 2) return false;
  for (int i = 2; i * i <= n; i++) {
    if (n % i == 0) {
      return false;
    }
  }
  return true;
}

std::vector<std::pair<int,int>> getJobList(int start, int end, int numWorkers) {
    // Write C++ code here
    int size = end - start + 1;
    std::vector<std::pair<int,int>> jobList;

    int jobChunk = size / numWorkers;
    int i = start;
    int j = start;
    int jobRemainder = size % numWorkers;
        while(size != 0){
            if(size - jobChunk > 0){
                j += jobChunk - 1;
                if(jobRemainder > 0){
                    ++j;
                    --jobRemainder;
                    --size;
                }
                jobList.push_back(std::pair(i,j));
                size -= jobChunk;
                i = j + 1;
                ++j;
            }else{
                j += size - 1;
                jobList.push_back(std::pair(i,j));
                size = 0;
            }
        }
    
    return jobList;
}
