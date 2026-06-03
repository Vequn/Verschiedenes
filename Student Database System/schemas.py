from pydantic import BaseModel, EmailStr, Field, validator
from typing import Optional

class StudentCreate(BaseModel):
    name: str = Field(..., min_length=2, max_length=100, description="Alphabetical characters only")
    email: EmailStr
    password: str = Field(..., min_length=8, max_length=64)
    gpa: Optional[float] = Field(None, ge=0.0, le=4.0)

    @validator('name')
    def validate_name(cls, v):
        if not all(x.isalpha() or x.isspace() for x in v):
            raise ValueError('Name must contain only letters and spaces')
        return v

class StudentResponse(BaseModel):
    id: int
    student_uuid: str
    name: str
    email: EmailStr
    gpa: Optional[float]

    class Config:
        from_attributes = True
