#include "DatabaseManager.hpp"
#include <stdexcept>
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& path, size_t initial_connections) 
    : db_path(path), pool_size(initial_connections) {
    
    std::lock_guard<std::mutex> lock(pool_mutex);
    for (size_t i = 0; i < pool_size; ++i) {
        connection_pool.push(create_new_connection());
    }
}

sqlite3* DatabaseManager::create_new_connection() {
    sqlite3* db;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database connection.");
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    return db;
}

sqlite3* DatabaseManager::acquire_connection() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    if (connection_pool.empty()) {
        // Dynamically grow pool if empty under heavy load
        return create_new_connection();
    }
    sqlite3* db = connection_pool.front();
    connection_pool.pop();
    return db;
}

void DatabaseManager::release_connection(sqlite3* db) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    connection_pool.push(db);
}

DatabaseManager::~DatabaseManager() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    while (!connection_pool.empty()) {
        sqlite3_close(connection_pool.front());
        connection_pool.pop();
    }
}
