#pragma once
#include <sqlite3.h>
#include <string>
#include <queue>
#include <mutex>
#include <memory>

class DatabaseManager {
private:
    std::string db_path;
    size_t pool_size;
    std::queue<sqlite3*> connection_pool;
    std::mutex pool_mutex;

    sqlite3* create_new_connection();

public:
    DatabaseManager(const std::string& path, size_t initial_connections = 10);
    ~DatabaseManager();

    // Get a connection from the pool
    sqlite3* acquire_connection();
    // Return a connection back to the pool
    void release_connection(sqlite3* db);
    
    void initialize_schema();
};
