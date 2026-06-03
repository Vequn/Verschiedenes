#include "crow.h"
#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// --- CORE RELATIONAL DATABASE ENGINE CLASS ---
class AcademicDatabase {
private:
    sqlite3* db;

    void initialize_schema() {
        // Enforce Foreign Key Constraints in SQLite
        sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

        // 1. Create Student Table
        const char* student_table = 
            "CREATE TABLE IF NOT EXISTS students ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "email TEXT UNIQUE NOT NULL);";
        
        // 2. Create Course Table
        const char* course_table = 
            "CREATE TABLE IF NOT EXISTS courses ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "course_code TEXT UNIQUE NOT NULL, "
            "title TEXT NOT NULL, "
            "credits INTEGER NOT NULL);";

        // 3. Create Enrollments Relational Lookup Table (Many-to-Many)
        const char* enrollment_table = 
            "CREATE TABLE IF NOT EXISTS enrollments ("
            "student_id INTEGER, "
            "course_id INTEGER, "
            "grade REAL CHECK(grade >= 0.0 AND grade <= 4.0), "
            "PRIMARY KEY (student_id, course_id), "
            "FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE, "
            "FOREIGN KEY(course_id) REFERENCES courses(id) ON DELETE CASCADE);";

        sqlite3_exec(db, student_table, nullptr, nullptr, nullptr);
        sqlite3_exec(db, course_table, nullptr, nullptr, nullptr);
        sqlite3_exec(db, enrollment_table, nullptr, nullptr, nullptr);
        
        // Seed initial dummy core data if database is empty
        seed_initial_data();
    }

    void seed_initial_data() {
        sqlite3_exec(db, "INSERT OR IGNORE INTO students (id, name, email) VALUES (1, 'Alice Vance', 'alice@university.edu');", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "INSERT OR IGNORE INTO courses (id, course_code, title, credits) VALUES (1, 'CS101', 'Introduction to Computer Science', 4);", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "INSERT OR IGNORE INTO courses (id, course_code, title, credits) VALUES (2, 'MATH201', 'Advanced Quantum Calculus', 3);", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "INSERT OR IGNORE INTO enrollments (student_id, course_id, grade) VALUES (1, 1, 4.0);", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "INSERT OR IGNORE INTO enrollments (student_id, course_id, grade) VALUES (1, 2, 3.6);", nullptr, nullptr, nullptr);
    }

public:
    AcademicDatabase(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Critical Error: Failed to open SQLite storage engine node.");
        }
        initialize_schema();
    }

    ~AcademicDatabase() {
        sqlite3_close(db);
    }

    // FEATURE 1: Relational Query System (Fetch Student Enrollment Matrix)
    crow::json::wvalue get_student_transcript(int student_id) {
        crow::json::wvalue transcript;
        const char* query = 
            "SELECT c.course_code, c.title, c.credits, e.grade "
            "FROM enrollments e "
            "JOIN courses c ON e.course_id = c.id "
            "WHERE e.student_id = ?;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
            transcript["error"] = "Database statement allocation failure.";
            return transcript;
        }

        // Securely bind the parameter to mitigate injection attacks
        sqlite3_bind_int(stmt, 1, student_id);

        int index = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            transcript["courses"][index]["code"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            transcript["courses"][index]["title"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            transcript["courses"][index]["credits"] = sqlite3_column_int(stmt, 2);
            transcript["courses"][index]["grade"] = sqlite3_column_double(stmt, 3);
            index++;
        }
        sqlite3_finalize(stmt);
        
        if (index == 0) {
            transcript["message"] = "No active course enrollments mapped to this profile ID.";
        }
        return transcript;
    }

    // FEATURE 2: Automated System Analytics Engine & Reporting (CSV Compiler)
    std::string generate_csv_performance_report() {
        std::stringstream csv_stream;
        // Build Data Header Layout
        csv_stream << "Student_ID,Name,Email,Total_Credits,Calculated_GPA\n";

        const char* aggregate_query = 
            "SELECT s.id, s.name, s.email, "
            "SUM(c.credits) AS total_credits, "
            "SUM(e.grade * c.credits) / SUM(c.credits) AS gpa "
            "FROM students s "
            "JOIN enrollments e ON s.id = e.student_id "
            "JOIN courses c ON e.course_id = c.id "
            "GROUP BY s.id;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, aggregate_query, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                std::string email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                int total_credits = sqlite3_column_int(stmt, 3);
                double gpa = sqlite3_
