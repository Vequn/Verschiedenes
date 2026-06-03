import sqlite3
from sqlite3 import Error

def create_connection():
    """Create a database connection to the SQLite database."""
    conn = None
    try:
        conn = sqlite3.connect("student_database.db")
        return conn
    except Error as e:
        print(f"Error connecting to database: {e}")
    return conn

def create_table(conn):
    """Create the students table if it doesn't exist."""
    try:
        sql_create_students_table = """
        CREATE TABLE IF NOT EXISTS students (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            age INTEGER NOT NULL,
            grade TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL
        );
        """
        cursor = conn.cursor()
        cursor.execute(sql_create_students_table)
    except Error as e:
        print(f"Error creating table: {e}")

# --- CRUD Operations ---

def add_student(conn):
    print("\n--- Add New Student ---")
    name = input("Enter Name: ").strip()
    try:
        age = int(input("Enter Age: "))
    except ValueError:
        print("Invalid age. Must be an integer.")
        return
    grade = input("Enter Grade/Class: ").strip()
    email = input("Enter Email: ").strip()

    sql = "INSERT INTO students(name, age, grade, email) VALUES(?,?,?,?)"
    try:
        cursor = conn.cursor()
        cursor.execute(sql, (name, age, grade, email))
        conn.commit()
        print(f"Success: Student added successfully with ID {cursor.lastrowid}!")
    except sqlite3.IntegrityError:
        print("Error: A student with this email already exists.")
    except Error as e:
        print(f"An error occurred: {e}")

def view_students(conn):
    print("\n--- Student Records ---")
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM students")
    rows = cursor.fetchall()

    if not rows:
        print("No student records found.")
        return

    # Print headers
    print(f"{'ID':<5} | {'Name':<20} | {'Age':<5} | {'Grade':<8} | {'Email':<25}")
    print("-" * 70)
    for row in rows:
        print(f"{row[0]:<5} | {row[1]:<20} | {row[2]:<5} | {row[3]:<8} | {row[4]:<25}")

def update_student(conn):
    print("\n--- Update Student ---")
    try:
        student_id = int(input("Enter Student ID to update: "))
    except ValueError:
        print("Invalid ID format.")
        return

    cursor = conn.cursor()
    cursor.execute("SELECT * FROM students WHERE id = ?", (student_id,))
    student = cursor.fetchone()

    if not student:
        print("Student record not found.")
        return

    print(f"Current Data -> Name: {student[1]}, Age: {student[2]}, Grade: {student[3]}, Email: {student[4]}")
    print("Leave field blank to keep current value.")

    new_name = input(f"New Name [{student[1]}]: ").strip() or student[1]
    new_age_input = input(f"New Age [{student[2]}]: ").strip()
    new_age = int(new_age_input) if new_age_input else student[2]
    new_grade = input(f"New Grade [{student[3]}]: ").strip() or student[3]
    new_email = input(f"New Email [{student[4]}]: ").strip() or student[4]

    sql = """ UPDATE students
              SET name = ? ,
                  age = ? ,
                  grade = ? ,
                  email = ?
              WHERE id = ?"""
    try:
        cursor.execute(sql, (new_name, new_age, new_grade, new_email, student_id))
        conn.commit()
        print("Success: Student record updated.")
    except sqlite3.IntegrityError:
        print("Error: That email is already in use by another student.")

def delete_student(conn):
    print("\n--- Delete Student ---")
    try:
        student_id = int(input("Enter Student ID to delete: "))
    except ValueError:
        print("Invalid ID format.")
        return

    cursor = conn.cursor()
    cursor.execute("SELECT * FROM students WHERE id = ?", (student_id,))
    if not cursor.fetchone():
        print("Student record not found.")
        return

    confirm = input(f"Are you sure you want to delete student ID {student_id}? (y/n): ").lower()
    if confirm == 'y':
        cursor.execute("DELETE FROM students WHERE id = ?", (student_id,))
        conn.commit()
        print("Success: Student record deleted.")
    else:
        print("Deletion canceled.")

# --- Main Application Loop ---

def main():
    conn = create_connection()
    if conn is not None:
        create_table(conn)
    else:
        print("Error! Cannot create the database connection.")
        return

    while True:
        print("\n==================================")
        print(" STUDENT DATABASE MANAGEMENT SYSTEM")
        print("==================================")
        print("1. Add Student")
        print("2. View All Students")
        print("3. Update Student")
        print("4. Delete Student")
        print("5. Exit")
        
        choice = input("Enter your choice (1-5): ").strip()

        if choice == '1':
            add_student(conn)
        elif choice == '2':
            view_students(conn)
        elif choice == '3':
            update_student(conn)
        elif choice == '4':
            delete_student(conn)
        elif choice == '5':
            print("\nClosing database connection... Goodbye!")
            conn.close()
            break
        else:
            print("Invalid choice! Please enter a number between 1 and 5.")

if __name__ == '__main__':
    main()
