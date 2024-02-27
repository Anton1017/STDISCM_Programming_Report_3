#include <iostream>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

int main() {
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

            // Split message into start, end, and num threads
            int i = 0;
            std::string temp = "";

            while (buffer[i] != '\0') {
                if (buffer[i] != ',') {
                    temp += buffer[i]; 
                } 
                else {
                    switch (i) {
                        case 0:
                            start = stoi(temp);
                            break;
                        case 1:
                            end = stoi(temp);
                            break;
                        case 2:
                            numThreads = stoi(temp);
                            break;
                        default:
                            break;
                    }
                    temp.clear();
                }
                i++;
            }

            // Check for termination message
            if (std::string(buffer) == "exit") {
                std::cout << "Client sent termination message" << std::endl;
                break;
            }
        }

        // Send a message to the connected client
        const char* message = "Hello from server!";
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
