#include "crow.h"
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <unordered_set>

// --- FIREWALL CONFIGURATION VAULTS ---
const std::unordered_set<std::string> IP_BLACKLIST = {
    "203.0.113.50", 
    "198.51.100.99"
};

const std::vector<std::string> BANNED_USER_AGENTS = {
    "sqlmap", "nikto", "nmap", "dirbuster", "gobuster", "masscan"
};

const std::vector<std::string> MALICIOUS_PATTERNS = {
    "(?i)<script.*?>.*?</script.*?>",               // Cross-Site Scripting (XSS) tags
    "(?i)javascript\\s*:",                           // Inline JS execution protocols
    "(?i)\\b(UNION\\s+SELECT|SELECT\\s+.*\\s+FROM)\\b", // SQL Injection reads
    "(?i)\\b(DROP\\s+TABLE|DELETE\\s+FROM|TRUNCATE\\s+TABLE)\\b", // Destructive SQLi
    "(--|/\\*|\\*/)"                                 // SQL Comments / Escape sequences
};

// --- CORE FIREWALL MIDDLEWARE LOGIC ---
struct ApplicationFirewall {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        std::string client_ip = req.remote_ip_address;
        std::string user_agent = req.get_header_value("User-Agent");
        std::string request_url = req.raw_url;

        // Transform user agent to lowercase for consistent signature matching
        std::transform(user_agent.begin(), user_agent.end(), user_agent.begin(), ::tolower);

        // 1. Inspect IP Address Authorization
        if (IP_BLACKLIST.find(client_ip) != IP_BLACKLIST.end()) {
            std::cout << "🚨 WAF ALERT: Terminated connection from blacklisted IP: " << client_ip << std::endl;
            res.code = 403;
            res.body = "Access Denied: Your IP address has been flagged by security.";
            res.end();
            return;
        }

        // 2. Inspect User-Agent Signatures
        for (const auto& pattern : BANNED_USER_AGENTS) {
            if (user_agent.find(pattern) != std::string::npos) {
                std::cout << "🚨 WAF REJECTION: Automated Scanner Blocked: " << user_agent << std::endl;
                res.code = 403;
                res.body = "Malicious User-Agent Blocked.";
                res.end();
                return;
            }
        }

        // 3. Deep URL Inspection (Scan path and query strings)
        for (const auto& pattern_str : MALICIOUS_PATTERNS) {
            std::regex pattern(pattern_str, std::regex_constants::icase);
            if (std::regex_search(request_url, pattern)) {
                std::cout << "🚨 WAF TRIGGERED: Malicious pattern found in request URL from IP " << client_ip << std::endl;
                res.code = 400;
                res.body = "Security Exception: Malicious payloads detected in request string.";
                res.end();
                return;
            }
        }

        // 4. Deep Payload Inspection (Scan incoming data bodies for POST/PUT requests)
        if (req.method == crow::HTTPMethod::POST || req.method == crow::HTTPMethod::PUT) {
            std::string body_str = req.body;
            for (const auto& pattern_str : MALICIOUS_PATTERNS) {
                std::regex pattern(pattern_str, std::regex_constants::icase);
                if (std::regex_search(body_str, pattern)) {
                    std::cout << "🚨 WAF TRIGGERED: Injection payload intercepted in POST body from IP " << client_ip << std::endl;
                    res.code = 400;
                    res.body = "Security Exception: Request body payload rejected.";
                    res.end();
                    return;
                }
            }
        }
        
        // If everything checks out, Crow moves the request to the designated route automatically.
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Post-execution monitoring loops can be safely attached here if needed
    }
};

// --- MAIN APPLICATION CONTAINER ---
int main() {
    // Initialize Crow with our custom Firewall middleware tracking module
    crow::App<ApplicationFirewall> app;

    // Route: Read Student Database Ledger
    CROW_ROUTE(app, "/api/v1/students")
    ([]() {
        crow::json::wvalue response_data;
        response_data[0]["id"] = 1;
        response_data[0]["name"] = "Alice Vance";
        response_data[0]["email"] = "alice@university.edu";
        response_data[0]["gpa"] = 3.9;
        return response_data;
    });

    // Route: Secure Creation Entrypoint
    CROW_ROUTE(app, "/api/v1/students").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto parsed_json = crow::json::load(req.body);
        if (!parsed_json) {
            return crow::response(400, "Invalid JSON structure payload.");
        }

        crow::json::wvalue confirmation;
        confirmation["status"] = "Success";
        confirmation["message"] = "Data compiled and committed securely to ledger system.";
        return crow::response(201, confirmation);
    });

    // Set port configuration, activate concurrency pooling, and launch the engine
    app.port(8000)
       .multithreaded()
       .run();
}
