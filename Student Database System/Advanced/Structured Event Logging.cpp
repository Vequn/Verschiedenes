#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>

class StructuralAuditLogger {
private:
    std::ofstream log_file;
    std::mutex log_mutex;

    std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    StructuralAuditLogger(const std::string& filename) {
        log_file.open(filename, std::ios::app);
    }

    ~StructuralAuditLogger() {
        if (log_file.is_open()) log_file.close();
    }

    void log_event(const std::string& tier, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file << "[" << get_current_timestamp() << "] "
                     << "[" << tier << "] " << message << "\n";
            log_file.flush(); // Force write to disk safely
        }
    }
};

// Global Instance instantiation inside main execution room
StructuralAuditLogger audit_trail("system_security_audit.log");
