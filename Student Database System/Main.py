from fastapi import FastAPI, Depends, HTTPException, status
from sqlalchemy.orm import Session
from database import SessionLocal, StudentModel, UserRole
import schemas
import auth

app = FastAPI(title="Advanced Secure Student DBMS", version="2.0.0")

# Dependency to safely handle DB sessions
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# Role Verifier Dependency
def require_admin_or_faculty(current_role: str = Depends(auth.get_current_user_role)):
    if current_role not in [UserRole.ADMIN, UserRole.FACULTY]:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, 
            detail="Operation restricted to Faculty/Admin accounts."
        )

@app.post("/students/register", response_model=schemas.StudentResponse, status_code=status.HTTP_201_CREATED)
def register_student(student: schemas.StudentCreate, db: Session = Depends(get_db)):
    # Check if student email is already registered
    db_student = db.query(StudentModel).filter(StudentModel.email == student.email).first()
    if db_student:
        raise HTTPException(status_code=400, detail="Email is already tied to a student record.")
    
    hashed_pwd = auth.get_password_hash(student.password)
    new_student = StudentModel(
        student_uuid=auth.generate_uuid(),
        name=student.name,
        email=student.email,
        gpa=student.gpa,
        hashed_password=hashed_pwd,
        role=UserRole.STUDENT
    )
    db.add(new_student)
    db.commit()
    db.refresh(new_student)
    return new_student

@app.get("/students/{student_id}", response_model=schemas.StudentResponse)
def get_student_profile(student_id: int, db: Session = Depends(get_db), current_role: str = Depends(auth.get_current_user_role)):
    # Any logged in user can see a profile, but structural endpoints can be gated downstream
    student = db.query(StudentModel).filter(StudentModel.id == student_id).first()
    if not student:
        raise HTTPException(status_code=404, detail="Student profile not found.")
    return student

@app.delete("/students/{student_id}", status_code=status.HTTP_204_NO_CONTENT, dependencies=[Depends(require_admin_or_faculty)])
def administrative_delete_student(student_id: int, db: Session = Depends(get_db)):
    student = db.query(StudentModel).filter(StudentModel.id == student_id).first()
    if not student:
        raise HTTPException(status_code=404, detail="Target record does not exist.")
    db.delete(student)
    db.commit()
    return None
