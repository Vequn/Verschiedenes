#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

using namespace std;

const int PORT = 8080;
const string LEADERBOARD_FILE = "casino_leaderboard.txt";

struct LeaderboardEntry {
    string name;
    int score;
};

bool compareEntries(const LeaderboardEntry& a, const LeaderboardEntry& b) {
    return a.score > b.score;
}

// Core Database Logic processed safely in isolation
string processDatabaseRequest(const string& request) {
    stringstream ss(request);
    string command, name;
    int chips;
    ss >> command >> name >> chips;

    if (command == "SAVE") {
        vector<LeaderboardEntry> entries;
        ifstream inFile(LEADERBOARD_FILE);
        bool found = false;

        if (inFile.is_open()) {
            string eName;
            int eScore;
            while (inFile >> eName >> eScore) {
                if (eName == name) {
                    if (chips > eScore) eScore = chips;
                    found = true;
                }
                entries.push_back({eName, eScore});
            }
            inFile.close();
        }

        if (!found) entries.push_back({name, chips});
        sort(entries.begin(), entries.end(), compareEntries);

        ofstream outFile(LEADERBOARD_FILE);
        if (outFile.is_open()) {
            for (const auto& entry : entries) {
                outFile << entry.name << " " << entry.score << "\n";
            }
            outFile.close();
        }
        return "SUCCESS_SAVED";
    }
    return "UNKNOWN_COMMAND";
}

int main() {
    cout << "[SERVER] Initializing High-Traffic Isolated Database Server...\n";

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    cout << "[SERVER] Database Listening seamlessly on Port " << PORT << "\n";

    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        char buffer[1024] = {0};
#ifdef _WIN32
        recv(client_socket, buffer, 1024, 0);
#else
        read(client_socket, buffer, 1024);
#endif

        string response = processDatabaseRequest(string(buffer));

#ifdef _WIN32
        send(client_socket, response.c_str(), response.length(), 0);
        closesocket(client_socket);
#else
        send(client_socket, response.c_str(), response.length(), 0);
        close(client_socket);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
    return 0;
}
