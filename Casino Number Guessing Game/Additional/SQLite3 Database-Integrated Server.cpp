#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <ctime>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <sqlite3.h> // <-- NEW: Native SQLite3 Library Integration

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
const string DB_FILE = "casino_ecosystem.db"; // SQLite Binary Database File

mutex dbMutex;
mutex firewallMutex;

// --- CRYPTOGRAPHIC ENGINE ---
class CryptoEngine {
private:
    static const char CIPHER_KEY = 0x5A;
public:
    static string decrypt(const string& cipherText) {
        string plainText = cipherText;
        for (size_t i = 0; i < plainText.size(); ++i) plainText[i] ^= CIPHER_KEY;
        return plainText;
    }
    static string encrypt(const string& plainText) {
        string cipherText = plainText;
        for (size_t i = 0; i < cipherText.size(); ++i) cipherText[i] ^= CIPHER_KEY;
        return cipherText;
    }
};

// --- FIREWALL REGISTRY ---
struct FirewallState {
    time_t lastRequestTime;
    int requestCount;
    int strikes;
    bool blacklisted;
};
map<string, FirewallState> networkBlacklist;

// ============================================================================
// SQL DATABASE CONTROLLER CLASS
// ============================================================================
class DatabaseController {
private:
    sqlite3* db;

public:
    DatabaseController() {
        // Open or create the binary database file
        if (sqlite3_open(DB_FILE.c_str(), &db) != SQLITE_OK) {
            cerr << "[DB ERROR] Failed to initialize SQLite engine: " << sqlite3_errmsg(db) << "\n";
            exit(1);
        }

        // Create the Leaderboard table dynamically if it doesn't exist
        string createTableSQL = 
            "CREATE TABLE IF NOT EXISTS leaderboard ("
            "username TEXT PRIMARY KEY NOT NULL, "
            "highest_balance INTEGER NOT NULL"
            ");";

        char* errorMessage = nullptr;
        if (sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            cerr << "[SQL ERROR] Table creation failed: " << errorMessage << "\n";
            sqlite3_free(errorMessage);
        } else {
            cout << "[DB ENGINE] Core SQLite Tables structural check complete.\n";
        }
    }

    // High-performance Upsert query (Update if exists, Insert if new)
    bool savePlayerProgress(const string& username, int currentChips) {
        // SQL query utilizing an UPSERT statement with parameter binding to prevent SQL Injection
        string query = 
            "INSERT INTO leaderboard (username, highest_balance) VALUES (?, ?) "
            "ON CONFLICT(username) DO UPDATE SET "
            "highest_balance = MAX(highest_balance, excluded.highest_balance);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        // Sanitize inputs by binding data parameters outside the compilation string
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, currentChips);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt); // Clean up memory statement handle
        return success;
    }

    ~DatabaseController() {
        sqlite3_close(db); // Gracefully close connections on termination
    }
};

// Global Database Controller Instance
DatabaseController* globalDB = nullptr;

// ============================================================================
// CONCURRENT WORKER POOL MANAGEMENT
// ============================================================================
class ThreadPool {
private:
    vector<thread> workers;
    queue<int> taskQueue;
    mutex queueMutex;
    condition_variable cv;
    bool stopPool;

    static void handleClientSession(int clientSocket, string clientIp) {
        char buffer[2048] = {0};
#ifdef _WIN32
        recv(clientSocket, buffer, 2048, 0);
#else
        read(clientSocket, buffer, 2048);
#endif

        string incomingRaw(buffer);
        string response;

        // 1. Firewall Access Token Evaluation
        bool trafficCleared = false;
        {
            lock_guard<mutex> lock(firewallMutex);
            time_t now = time(0);
            if (!networkBlacklist[clientIp].blacklisted) {
                if (now - networkBlacklist[clientIp].lastRequestTime < 1) {
                    networkBlacklist[clientIp].requestCount++;
                } else {
                    networkBlacklist[clientIp].requestCount = 1;
                    networkBlacklist[clientIp].lastRequestTime = now;
                }
                if (networkBlacklist[clientIp].requestCount > 10) {
                    networkBlacklist[clientIp].strikes++;
                    if (networkBlacklist[clientIp].strikes >= 3) networkBlacklist[clientIp].blacklisted = true;
                } else {
                    trafficCleared = true;
                }
            }
        }

        if (!trafficCleared) {
            response = CryptoEngine::encrypt("ERROR_FIREWALL_REJECTION");
        } else {
            // 2. Cryptographic Processing
            string decryptedPayload = CryptoEngine::decrypt(incomingRaw);
            stringstream ss(decryptedPayload);
            string command, name;
            int chips;
            
            if (!(ss >> command >> name >> chips) || command != "SAVE") {
                response = CryptoEngine::encrypt("ERROR_MALFORMED_DATA_PACKET");
            } else {
                // 3. Thread-Safe Atomic SQL Database Execution
                bool dbSuccess = false;
                {
                    lock_guard<mutex> dbLock(dbMutex);
                    dbSuccess = globalDB->savePlayerProgress(name, chips);
                }

                if (dbSuccess) {
                    response = CryptoEngine::encrypt("SUCCESS_SQL_LEDGER_SYNCED");
                } else {
                    response = CryptoEngine::encrypt("ERROR_SQL_TRANSACTION_FAILED");
                }
            }
        }

#ifdef _WIN32
        send(clientSocket, response.c_str(), response.length(), 0);
        closesocket(clientSocket);
#else
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
#endif
    }

public:
    ThreadPool(size_t threads) : stopPool(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.push_back(thread([this]() {
                while (true) {
                    int clientSocket;
                    string clientIp;
                    {
                        unique_lock<mutex> lock(this->queueMutex);
                        this->cv.wait(lock, [this]() { return this->stopPool || !this->taskQueue.empty(); });
                        if (this->stopPool && this->taskQueue.empty()) return;
                        clientSocket = this->taskQueue.front();
                        this->taskQueue.pop();
                    }
                    
                    struct sockaddr_in addr;
                    int addrSize = sizeof(addr);
                    getpeername(clientSocket, (struct sockaddr*)&addr, (socklen_t*)&addrSize);
                    clientIp = string(inet_ntoa(addr.sin_addr));

                    handleClientSession(clientSocket, clientIp);
                }
            }));
        }
    }

    void enqueue(int socketDescriptor) {
        {
            lock_guard<mutex> lock(queueMutex);
            taskQueue.push(socketDescriptor);
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        { lock_guard<mutex> lock(queueMutex); stopPool = true; }
        cv.notify_all();
        for (thread &worker : workers) { if (worker.joinable()) worker.join(); }
    }
};

// ============================================================================
// MAIN RECONFIGURED ORCHESTRATOR
// ============================================================================
int main() {
    cout << "[ENTERPRISE SERVER] Booting SQL Database and Crypto Network Subsystems...\n";

    // Instantiate SQL Engine Instance
    globalDB = new DatabaseController();

#ifdef _WIN32
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 50);

    ThreadPool pool(4);
    cout << "[ENTERPRISE SERVER] Active SQL Engine Ready on Port " << PORT << ".\n";

    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        pool.enqueue(client_socket);
    }

    delete globalDB;
#ifdef _WIN32
    closesocket(server_fd); WSACleanup();
#else
    close(server_fd);
#endif
    return 0;
}
