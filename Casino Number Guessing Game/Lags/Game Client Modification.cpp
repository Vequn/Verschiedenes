#include <iostream>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

using namespace std;

// This replaces the old file writer in your game engine
void sendDataToDataServer(const string& playerName, int currentBalance) {
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    
    // Connect to local host interface
    #ifdef _WIN32
        InetPton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    #else
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    #endif

    // Non-blocking network ping connection
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0) {
        // Build server-interpretable command packet string
        string dataPacket = "SAVE " + playerName + " " + to_string(currentBalance);
        
        #ifdef _WIN32
            send(sock, dataPacket.c_str(), dataPacket.length(), 0);
            closesocket(sock);
        #else
            send(sock, dataPacket.c_str(), dataPacket.length(), 0);
            close(sock);
        #endif
    } else {
        cout << "[Client Network Error] Could not reach Data Engine Server. Saving locally into cached memory.\n";
    }

    #ifdef _WIN32
        WSACleanup();
    #endif
}
