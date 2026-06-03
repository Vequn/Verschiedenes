from fastapi.middleware.cors import CORSMiddleware

# Paste this right below your app = FastAPI(...) initialization
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # Allow any local development origin server file to hit endpoints
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Enforce an explicit record-fetch route for the script matrix
@app.get("/students/all", response_model=list[schemas.StudentResponse])
def read_all_students(db: Session = Depends(get_db)):
    return db.query(StudentModel).all()
