#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#include <ctime>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

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

// Thread Safety Guardrails
mutex dbMutex;
mutex firewallMutex;

// --- CRYPTOGRAPHIC SIMULATION LAYER ---
// In a true enterprise environment, you would link OpenSSL (-lssl -lcrypto).
// This inner class simulates a symmetric cipher (XOR-Rotator) acting as our AES-256 pipeline.
class CryptoEngine {
private:
    static const char CIPHER_KEY = 0x5A; // Secret server-side private key
public:
    static string decrypt(const string& cipherText) {
        string plainText = cipherText;
        for (size_t i = 0; i < plainText.size(); ++i) {
            plainText[i] ^= CIPHER_KEY; 
        }
        return plainText;
    }
    static string encrypt(const string& plainText) {
        string cipherText = plainText;
        for (size_t i = 0; i < cipherText.size(); ++i) {
            cipherText[i] ^= CIPHER_KEY;
        }
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
// CONCURRENT WORKER POOL MANAGEMENT
// ============================================================================
class ThreadPool {
private:
    vector<thread> workers;
    queue<int> taskQueue;
    mutex queueMutex;
    condition_variable cv;
    bool stopPool;

    // Direct structural reference to database processing logic
    static void handleClientSession(int clientSocket, string clientIp) {
        char buffer[2048] = {0};
#ifdef _WIN32
        recv(clientSocket, buffer, 2048, 0);
#else
        read(clientSocket, buffer, 2048);
#endif

        string incomingRaw(buffer);
        string response;

        // 1. Thread-Safe Firewall Interception
        bool trafficCleared = false;
        {
            lock_guard<mutex> lock(firewallMutex);
            time_t now = time(0);
            
            if (networkBlacklist[clientIp].blacklisted) {
                trafficCleared = false;
            } else {
                // Rate Limiting Evaluation (Max 10 requests per second per thread node)
                if (now - networkBlacklist[clientIp].lastRequestTime < 1) {
                    networkBlacklist[clientIp].requestCount++;
                } else {
                    networkBlacklist[clientIp].requestCount = 1;
                    networkBlacklist[clientIp].lastRequestTime = now;
                }

                if (networkBlacklist[clientIp].requestCount > 10) {
                    networkBlacklist[clientIp].strikes++;
                    if (networkBlacklist[clientIp].strikes >= 3) networkBlacklist[clientIp].blacklisted = true;
                    trafficCleared = false;
                } else {
                    trafficCleared = true;
                }
            }
        }

        if (!trafficCleared) {
            response = CryptoEngine::encrypt("ERROR_FIREWALL_REJECTION");
        } else {
            // 2. Cryptographic Decryption
            string decryptedPayload = CryptoEngine::decrypt(incomingRaw);
            
            stringstream ss(decryptedPayload);
            string command, name;
            int chips;
            
            if (!(ss >> command >> name >> chips) || command != "SAVE") {
                response = CryptoEngine::encrypt("ERROR_MALFORMED_DATA_PACKET");
            } else {
                // 3. Thread-Safe Atomic Database Sync
                lock_guard<mutex> dbLock(dbMutex);
                vector<pair<string, int>> ledger;
                ifstream inFile(LEADERBOARD_FILE);
                bool found = false;

                if (inFile.is_open()) {
                    string rName; int rScore;
                    while (inFile >> rName >> rScore) {
                        if (rName == name) {
                            if (chips > rScore) rScore = chips;
                            found = true;
                        }
                        ledger.push_back({rName, rScore});
                    }
                    inFile.close();
                }
                if (!found) ledger.push_back({name, chips});
                
                sort(ledger.begin(), ledger.end(), [](const pair<string, int>& a, const pair<string, int>& b){
                    return a.second > b.second;
                });

                ofstream outFile(LEADERBOARD_FILE);
                if (outFile.is_open()) {
                    for (const auto& row : ledger) outFile << row.first << " " << row.second << "\n";
                    outFile.close();
                }
                response = CryptoEngine::encrypt("SUCCESS_LEDGER_SYNCED");
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
                    
                    // Fallback configuration to extract IP dynamically inside worker context
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
        {
            lock_guard<mutex> lock(queueMutex);
            stopPool = true;
        }
        cv.notify_all();
        for (thread &worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
};

// ============================================================================
// MAIN SYSTEM ORCHESTRATION
// ============================================================================
int main() {
    cout << "[ENTERPRISE SERVER] Booting Asynchronous Crypto-Engine Network Subsystem...\n";

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
    listen(server_fd, 50); // Buffers up to 50 structural listener attachments

    // Initialize 4 highly optimized processing worker threads permanently residing in RAM memory
    ThreadPool pool(4);
    cout << "[ENTERPRISE SERVER] Server Pools loaded. Dynamic Symmetric Crypto-Verification fully Active.\n";

    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        
        // Pass off raw descriptor handles instantly to the background engine pool threads
        pool.enqueue(client_socket);
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
    return 0;
}
