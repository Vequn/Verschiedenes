#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <ctime>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sqlite3.h>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <fcntl.h>
#endif

using namespace std;

const int PORT = 8080;
const int PING_INTERVAL_SEC = 5;
const int MAX_MISSED_PINGS = 3;

mutex clientsMutex;
mutex dbMutex;

struct ClientSession {
    int socketDescriptor;
    string ipAddress;
    string username;
    int missedPings;
    time_t lastSeen;
    bool active;
};

// Global list of currently connected, persistent player sessions
vector<ClientSession*> connectedClients;
sqlite3* dbHandle;

// Helper to set non-blocking sockets across platforms
void setNonBlocking(int socketFd) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(socketFd, FIONBIO, &mode);
#else
    int flags = fcntl(socketFd, F_GETFL, 0);
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
#endif
}

// --- DATABASE LAYER ---
void initDatabase() {
    sqlite3_open("casino_ecosystem.db", &dbHandle);
    string sql = "CREATE TABLE IF NOT EXISTS leaderboard (username TEXT PRIMARY KEY, highest_balance INTEGER);";
    sqlite3_exec(dbHandle, sql.c_str(), nullptr, nullptr, nullptr);
}

void saveToDatabase(const string& username, int chips) {
    lock_guard<mutex> lock(dbMutex);
    string query = "INSERT INTO leaderboard (username, highest_balance) VALUES (?, ?) "
                   "ON CONFLICT(username) DO UPDATE SET highest_balance = MAX(highest_balance, excluded.highest_balance);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(dbHandle, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, chips);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

// ============================================================================
// HEARTBEAT MONITOR THREAD (Runs indefinitely in the background)
// ============================================================================
void heartbeatMonitorLoop() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(PING_INTERVAL_SEC));
        lock_guard<mutex> lock(clientsMutex);

        for (auto it = connectedClients.begin(); it != connectedClients.end(); ) {
            ClientSession* session = *it;

            if (!session->active) {
                it = connectedClients.erase(it);
                delete session;
                continue;
            }

            // If client missed too many pings, terminate the dead connection dead connection drop
            if (session->missedPings >= MAX_MISSED_PINGS) {
                cout << "🚨 [HEARTBEAT TIMEOUT] Client " << session->username 
                     << " (" << session->ipAddress << ") dropped connection. Cleaning up raw structures.\n";
                
#ifdef _WIN32
                closesocket(session->socketDescriptor);
#else
                close(session->socketDescriptor);
#endif
                session->active = false;
                it = connectedClients.erase(it);
                delete session;
            } else {
                // Send Ping packet command stream down the socket pipeline
                string pingMsg = "PING\n";
                send(session->socketDescriptor, pingMsg.c_str(), pingMsg.length(), 0);
                session->missedPings++;
                ++it;
            }
        }
    }
}

// ============================================================================
// CLIENT SESSION THREAD (One dedicated light thread per active player connection)
// ============================================================================
void processPersistentClient(ClientSession* session) {
    char buffer[1024];
    setNonBlocking(session->socketDescriptor);

    while (session->active) {
        memset(buffer, 0, sizeof(buffer));
#ifdef _WIN32
        int bytesRead = recv(session->socketDescriptor, buffer, sizeof(buffer) - 1, 0);
#else
        int bytesRead = read(session->socketDescriptor, buffer, sizeof(buffer) - 1);
#endif

        if (bytesRead > 0) {
            string incoming(buffer);
            stringstream ss(incoming);
            string command;
            ss >> command;

            // Handle Heartbeat Response
            if (command == "PONG") {
                session->missedPings = 0; // Reset missing counter instantly
                session->lastSeen = time(0);
            } 
            // Handle Gameplay Data Save
            else if (command == "SAVE") {
                string name; int chips;
                ss >> name >> chips;
                session->username = name; // Map connection handle to user profile
                saveToDatabase(name, chips);
                string response = "ACK_SAVED\n";
                send(session->socketDescriptor, response.c_str(), response.length(), 0);
            }
        } 
        else if (bytesRead == 0) {
            // Client closed connection gracefully
            cout << "ℹ️ [DISCONNECT] Player " << session->username << " disconnected gracefully.\n";
            session->active = false;
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(100)); // Relieve CPU thread scaling load
    }
}

// ============================================================================
// MAIN SERVER MANAGEMENT ORCHESTRATION
// ============================================================================
int main() {
    cout << "[PLATFORM SERVER] Activating Persistent Infrastructure Engine with Active Heartbeat Tracking...\n";
    initDatabase();

#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 20);

    // Spawn the background global health monitoring supervisor execution path
    thread monitorThread(heartbeatMonitorLoop);
    monitorThread.detach();

    cout << "[PLATFORM SERVER] Network Matrix listening for persistent game streams on port " << PORT << "\n";

    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        ClientSession* newSession = new ClientSession();
        newSession->socketDescriptor = client_socket;
        newSession->ipAddress = string(inet_ntoa(address.sin_addr));
        newSession->username = "Anonymous";
        newSession->missedPings = 0;
        newSession->lastSeen = time(0);
        newSession->active = true;

        {
            lock_guard<mutex> lock(clientsMutex);
            connectedClients.push_back(newSession);
        }

        // Spawn a standalone micro-worker thread execution map to handle persistent client reads
        thread clientThread(processPersistentClient, newSession);
        clientThread.detach();
    }

    sqlite3_close(dbHandle);
    return 0;
}
