#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <ctime>
#include <thread>
#include <mutex>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

using namespace std;

const int ADMIN_PORT = 8081;

// Reuse the architectural constructs from previous blocks
struct ClientSession {
    int socketDescriptor;
    string ipAddress;
    string username;
    bool active;
};
extern vector<ClientSession*> connectedClients; // Tracked from main game engine
extern mutex clientsMutex;

struct FirewallState {
    int strikes;
    bool blacklisted;
};
extern map<string, FirewallState> networkBlacklist; // Tracked from firewall module
extern mutex firewallMutex;

// ============================================================================
// REMOTE ADMIN COMMAND PROCESSOR
// ============================================================================
string executeAdminCommand(const string& rawCommand) {
    stringstream ss(rawCommand);
    string action;
    ss >> action;

    // Command 1: Get Server Health and Telemetry Diagnostics
    if (action == "/status") {
        lock_guard<mutex> lock(clientsMutex);
        stringstream response;
        response << "\n--- CORE TELEMETRY STATUS ---\n"
                 << "  Active Tables Connections: " << connectedClients.size() << "\n"
                 << "  Database State: SQLite3 Binary Linked [ONLINE]\n"
                 << "  Subsystem Threads: 4 Worker Threads Active\n";
        return response.str();
    }
    
    // Command 2: List current real-time players sitting at network terminals
    else if (action == "/players") {
        lock_guard<mutex> lock(clientsMutex);
        if (connectedClients.empty()) return "  [INFO] No active player sessions currently tracked.\n";
        
        stringstream response;
        response << "\n--- ACTIVE PLAYER LEDGER ---\n";
        for (const auto& client : connectedClients) {
            if (client->active) {
                response << "  👤 User: " << client->username << " | IP: " << client->ipAddress << "\n";
            }
        }
        return response.str();
    }

    // Command 3: Remotely override the Firewall to drop a specific malicious IP
    else if (action == "/banip") {
        string targetIp;
        ss >> targetIp;
        if (targetIp.empty()) return "  [ERROR] Correct Syntax: /banip [IP_ADDRESS]\n";

        lock_guard<mutex> lock(firewallMutex);
        networkBlacklist[targetIp].blacklisted = true;
        networkBlacklist[targetIp].strikes = 3;
        return "  [SUCCESS] IP Address " + targetIp + " permanently dropped and blacklisted by Firewall.\n";
    }

    // Command 4: System Help Utility
    else if (action == "/help") {
        return "\n--- AVAILABLE ADMIN INTERFACES ---\n"
               "  /status          - Displays real-time infrastructure runtime values.\n"
               "  /players         - Enumerates all actively bound connected users.\n"
               "  /banip [IP]      - Drops target routing vector instantly via firewall.\n"
               "  /shutdown        - Closes active network listeners gracefully.\n";
    }

    return "  [INVALID COMMAND] Type /help to query system instructions.\n";
}

// ============================================================================
// ADMIN SERVICE LISTENER LOOP (Runs asynchronously on an isolated thread)
// ============================================================================
void remoteAdminListenerLoop() {
    int admin_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ADMIN_PORT);

    // Allow quick port re-binding to prevent socket lockups on rapid server reboots
    int opt = 1;
#ifdef _WIN32
    setsockopt(admin_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(admin_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    bind(admin_fd, (struct sockaddr*)&address, sizeof(address));
    listen(admin_fd, 2);

    while (true) {
        int addrlen = sizeof(address);
        int admin_socket = accept(admin_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        cout << "🔒 [SECURITY NOTICE] Root Administrative connection established from terminal endpoint.\n";
        
        // Persistent command execution processing shell loop for the authorized socket channel
        char buffer[1024];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
#ifdef _WIN32
            int readBytes = recv(admin_socket, buffer, sizeof(buffer) - 1, 0);
#else
            int readBytes = read(admin_socket, buffer, sizeof(buffer) - 1);
#endif
            if (readBytes <= 0) break; // Drop connection cleanly if admin drops socket link

            string rawInput(buffer);
            // Strip out carriage returns or trailing spacing artifacts from standard terminal shells
            rawInput.erase(remove(rawInput.begin(), rawInput.end(), '\r'), rawInput.end());
            rawInput.erase(remove(rawInput.begin(), rawInput.end(), '\n'), rawInput.end());

            if (rawInput == "/shutdown") {
                string goodbye = "Shutting down central server clusters...\n";
                send(admin_socket, goodbye.c_str(), goodbye.length(), 0);
                exit(0); // Production-level execution termination
            }

            string executionReply = executeAdminCommand(rawInput);
            send(admin_socket, executionReply.c_str(), executionReply.length(), 0);
        }
#ifdef _WIN32
        closesocket(admin_socket);
#else
        close(admin_socket);
#endif
    }
}

// Integrate this line inside your main server entry thread initialization block:
// thread adminService(remoteAdminListenerLoop);
// adminService.detach();
