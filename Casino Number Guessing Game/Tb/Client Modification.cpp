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

// Matching cryptographic key used by the standalone cluster server
const char SECURE_KEY = 0x5A;

string encryptPayload(const string& data) {
    string cipher = data;
    for(size_t i = 0; i < cipher.size(); ++i) cipher[i] ^= SECURE_KEY;
    return cipher;
}

void secureTransmitData(const string& name, int score) {
    #ifdef _WIN32
        WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    #endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    
    #ifdef _WIN32
        InetPton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    #else
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    #endif

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0) {
        // Construct transaction data and convert to ciphertext
        string rawMessage = "SAVE " + name + " " + to_string(score);
        string securePacket = encryptPayload(rawMessage);
        
        #ifdef _WIN32
            send(sock, securePacket.c_str(), securePacket.length(), 0);
            closesocket(sock);
        #else
            send(sock, securePacket.c_str(), securePacket.length(), 0);
            close(sock);
        #endif
    }
    #ifdef _WIN32
        WSACleanup();
    #endif
}
