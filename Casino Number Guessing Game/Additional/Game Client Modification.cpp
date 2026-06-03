#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

using namespace std;

int globalSocketFd = -1;
bool isNetworkActive = false;

// Asynchronous background network supervisor loop for the Client application
void cleanNetworkReaderLoop() {
    char buffer[512];
    while (isNetworkActive) {
        memset(buffer, 0, sizeof(buffer));
#ifdef _WIN32
        int bytesReceived = recv(globalSocketFd, buffer, sizeof(buffer) - 1, 0);
#else
        int bytesReceived = read(globalSocketFd, buffer, sizeof(buffer) - 1);
#endif

        if (bytesReceived > 0) {
            string incoming(buffer);
            // Intercept and auto-process background heartbeat requests without disturbing the player
            if (incoming.find("PING") != string::npos) {
                string pongResponse = "PONG\n";
                send(globalSocketFd, pongResponse.c_str(), pongResponse.length(), 0);
            } else if (incoming.find("ACK_SAVED") != string::npos) {
                // Handle progress confirmation flags if necessary
            }
        } else if (bytesReceived == 0) {
            cout << "\n🚨 [NETWORK ALERT] Connection lost to game servers! Progress is running on temporary memory.\n";
            isNetworkActive = false;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(200));
    }
}

// Establishes a permanent websocket-style pipe to the gaming infrastructure server cluster
void connectToGameServer() {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    globalSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
#ifdef _WIN32
    InetPton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
#else
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
#endif

    if (connect(globalSocketFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0) {
        isNetworkActive = true;
        // Detach background worker to absorb network traffic asynchronously
        thread networkReceiver(cleanNetworkReaderLoop);
        networkReceiver.detach();
        cout << "[SYSTEM] Safely connected to persistent master game cluster server.\n";
    } else {
        cout << "[SYSTEM ERROR] Offline mode triggered. Infrastructure is unreachable.\n";
    }
}

void triggerManualSave(const string& playerName, int currentBalance) {
    if (!isNetworkActive) return;
    string packet = "SAVE " + playerName + " " + to_string(currentBalance) + "\n";
    send(globalSocketFd, packet.c_str(), packet.length(), 0);
}
