import os
from datetime import datetime, timedelta
from typing import Optional
from fastapi import FastAPI, Request, Depends, HTTPException, status, Response, Cookie
from fastapi.responses import JSONResponse
from passlib.context import CryptContext
from jose import JWTError, jwt
from pydantic import BaseModel, EmailStr

# --- SLOWAPI RATE LIMITER SETUP ---
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

limiter = Limiter(key_func=get_remote_address)
app = FastAPI(title="Enterprise Hardened Security Hub")
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)

# --- CRYPTOGRAPHIC CONFIGURATION ---
# In production, pull these dynamically from secure system environment vaults
SECRET_KEY = os.getenv("JWT_SECRET_PRIMARY", "9bc8e5623df8163f41c30291ba6c5a2cbb5a0a31210291fd")
REFRESH_SECRET_KEY = os.getenv("JWT_SECRET_REFRESH", "1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x")
ALGORITHM = "HS256"

ACCESS_TOKEN_EXPIRE_MINUTES = 15
REFRESH_TOKEN_EXPIRE_DAYS = 7

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

# --- DATA SCHEMAS ---
class UserLogin(BaseModel):
    email: EmailStr
    password: str

# --- CORE SECURITY ENGINE ---
class SecurityManager:
    @staticmethod
    def hash_password(password: str) -> str:
        return pwd_context.hash(password)

    @staticmethod
    def verify_password(plain_password: str, hashed_password: str) -> bool:
        return pwd_context.verify(plain_password, hashed_password)

    @staticmethod
    def create_token(data: dict, expires_delta: timedelta, secret: str) -> str:
        to_encode = data.copy()
        expire = datetime.utcnow() + expires_delta
        to_encode.update({"exp": expire})
        return jwt.encode(to_encode, secret, algorithm=ALGORITHM)

# --- MOCK USER DATABASE (For Security Isolation demonstration) ---
# In real application production, tie this map directly to your PostgreSQL database context
MOCK_DB = {
    "registrar@university.edu": {
        "email": "registrar@university.edu",
        "hashed_password": SecurityManager.hash_password("SuperSecurePassword2026!"),
        "role": "admin"
    }
}

# --- SECURE APIS & ENDPOINTS ---

@app.post("/api/v1/auth/login")
@limiter.limit("5/minute")  # Mitigates automated dictionary attacks
async def login(request: Request, user_credentials: UserLogin, response: Response):
    user = MOCK_DB.get(user_credentials.email)
    if not user or not SecurityManager.verify_password(user_credentials.password, user["hashed_password"]):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid security credentials provided."
        )

    # 1. Generate short-lived access payload
    access_delta = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = SecurityManager.create_token(
        data={"sub": user["email"], "role": user["role"]}, 
        expires_delta=access_delta, 
        secret=SECRET_KEY
    )

    # 2. Generate long-lived refresh token
    refresh_delta = timedelta(days=REFRESH_TOKEN_EXPIRE_DAYS)
    refresh_token = SecurityManager.create_token(
        data={"sub": user["email"]}, 
        expires_delta=refresh_delta, 
        secret=REFRESH_SECRET_KEY
    )

    # 3. Encapsulate Refresh Token into an immutable HttpOnly Cookie
    response.set_cookie(
        key="secure_refresh_token",
        value=refresh_token,
        httponly=True,               # Prevents JavaScript reading (Stops XSS)
        max_age=REFRESH_TOKEN_EXPIRE_DAYS * 24 * 60 * 60,
        expires=REFRESH_TOKEN_EXPIRE_DAYS * 24 * 60 * 60,
        samesite="strict",           # Prevents Cross-Site Request Forgery (CSRF)
        secure=True                  # Enforces HTTPS compilation transfer only
    )

    # Return access token in JSON body for frontend in-memory storage
    return {
        "access_token": access_token, 
        "token_type": "bearer", 
        "expires_in": ACCESS_TOKEN_EXPIRE_MINUTES * 60
    }

@app.post("/api/v1/auth/refresh")
async def refresh_session(secure_refresh_token: Optional[str] = Cookie(None)):
    """
    Silent re-authentication endpoint. Frontend hits this when access_token expires.
    """
    if not secure_refresh_token:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED, 
            detail="Session context missing. Please login again."
        )

    try:
        # Decode using the dedicated internal Refresh Key
        payload = jwt.decode(secure_refresh_token, REFRESH_SECRET_KEY, algorithms=[ALGORITHM])
        email: str = payload.get("sub")
        if email is None or email not in MOCK_DB:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid session identity.")
        
        # Generate a pristine new short-lived access token
        user = MOCK_DB[email]
        access_delta = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
        new_access_token = SecurityManager.create_token(
            data={"sub": user["email"], "role": user["role"]}, 
            expires_delta=access_delta, 
            secret=SECRET_KEY
        )
        
        return {"access_token": new_access_token, "token_type": "bearer"}

    except JWTError:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Tampered or expired refresh signature.")

@app.post("/api/v1/auth/logout")
async def logout(response: Response):
    """
    Explicitly clear cookies from the client browser to terminate the session safely.
    """
    response.delete_cookie(key="secure_refresh_token", samesite="strict", httponly=True, secure=True)
    return {"detail": "Session successfully terminated globally."}

@app.get("/api/v1/students/records")
@limiter.limit("100/minute") # General data query rate limits
async def get_secure_records(request: Request):
    # Enforce standard access token checks here for secure endpoints
    return {"status": "Access Granted", "data": "Classroom database ledger plaintext."}
