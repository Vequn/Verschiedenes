import json
import os
from typing import List, Optional
from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, EmailStr, Field
import redis
from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker, AsyncSession
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column
from sqlalchemy import select

# --- SYSTEM INITIALIZATION & APP CONFIG ---
app = FastAPI(title="Enterprise Infrastructure Student DBMS", version="3.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

DATABASE_URL = os.getenv("DATABASE_URL", "postgresql+asyncpg://admin_user:SecureDbPassword2026!@localhost:5432/secure_student_db")
REDIS_URL = os.getenv("REDIS_URL", "redis://localhost:6379/0")

# Asynchronous DB Engine Setup
async_engine = create_async_engine(DATABASE_URL, echo=False, pool_size=20, max_overflow=10)
AsyncSessionLocal = async_sessionmaker(bind=async_engine, autoflush=False, autocommit=False)

# Sync Redis client for blazing fast serialization 
redis_client = redis.from_url(REDIS_URL, decode_responses=True)

# --- DATABASE MODELS ---
class Base(DeclarativeBase):
    pass

class Student(Base):
    __tablename__ = "students"
    
    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    name: Mapped[str] = mapped_column(index=True)
    email: Mapped[str] = mapped_column(unique=True, index=True)
    gpa: Mapped[float] = mapped_column(nullable=True)

# --- DATA SCHEMAS (PYDANTIC) ---
class StudentCreate(BaseModel):
    name: str = Field(..., min_length=2, max_length=100)
    email: EmailStr
    gpa: Optional[float] = Field(None, ge=0.0, le=4.0)

class StudentResponse(BaseModel):
    id: int
    name: str
    email: EmailStr
    gpa: Optional[float]

    class Config:
        from_attributes = True

# --- LIFECYCLE DEPENDENCIES ---
async def get_db_session():
    async with AsyncSessionLocal() as session:
        try:
            yield session
        finally:
            await session.close()

@app.on_event("startup")
async def startup_event():
    # Automatically seed the database schemas to Postgres on spin up
    async with async_engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

# --- CACHE-ASIDE READ OPERATIONS ---
@app.get("/students", response_model=List[StudentResponse])
async def get_all_students(db: AsyncSession = Depends(get_db_session)):
    cache_key = "students:all"
    
    # 1. Attempt Cache Lookup
    try:
        cached_data = redis_client.get(cache_key)
        if cached_data:
            print("⚡ CACHE HIT: Returning ledger from Redis Memory Space.")
            return json.loads(cached_data)
    except redis.RedisError as re:
        print(f"⚠️ Redis unavailable: {re}")

    # 2. Cache Miss: Execute Asynchronous DB Query
    print("💾 CACHE MISS: Querying live system PostgreSQL Cluster.")
    result = await db.execute(select(Student))
    students = result.scalars().all()
    
    # Serialize model query objects to JSON format
    response_payload = [
        {"id": s.id, "name": s.name, "email": s.email, "gpa": s.gpa} for s in students
    ]
    
    # 3. Store in Redis Cache with a 60-second Time-To-Live (TTL)
    try:
        redis_client.setex(cache_key, 60, json.dumps(response_payload))
    except redis.RedisError:
        pass

    return response_payload

# --- MUTATION OPERATIONS (CACHE INVALIDATION) ---
@app.post("/students", response_model=StudentResponse, status_code=status.HTTP_201_CREATED)
async def create_student(student: StudentCreate, db: AsyncSession = Depends(get_db_session)):
    # Validate uniqueness
    existing_check = await db.execute(select(Student).where(Student.email == student.email))
    if existing_check.scalar_one_or_none():
        raise HTTPException(status_code=400, detail="Data constraint failure: Unique email violation.")

    new_student = Student(name=student.name, email=student.email, gpa=student.gpa)
    db.add(new_student)
    await db.commit()
    await db.refresh(new_student)

    # CRITICAL: Invalidate the stale cache so subsequent requests read the updated data
    try:
        redis_client.delete("students:all")
        print("🧹 CACHE INVALIDATED: Stale ledger purged from memory room.")
    except redis.RedisError:
        pass

    return new_student
