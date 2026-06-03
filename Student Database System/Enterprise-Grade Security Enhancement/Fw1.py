import re
from fastapi import FastAPI, Request, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.responses import Response

app = FastAPI(title="Enterprise Firewalled Student DBMS")

# --- FIREWALL CONFIGURATION VAULTS ---
# IPs explicitly banned from communicating with our database network
IP_BLACKLIST = {"203.0.113.50", "198.51.100.99"} 

# Automated security scanners we want to block instantly
BANNED_USER_AGENTS = [
    r"sqlmap", r"nikto", r"nmap", r"dirbuster", r"gobuster", r"masscan"
]

# Regex patterns to catch cross-site scripting (XSS) and SQL Injection (SQLi)
MALICIOUS_PATTERNS = [
    r"(?i)<script.*?>.*?</script.*?>",  # Basic XSS tags
    r"(?i)javascript\s*:",              # JavaScript protocol URI execution
    r"(?i)\b(UNION\s+SELECT|SELECT\s+.*\s+FROM)\b", # SQL Injection Selects
    r"(?i)\b(DROP\s+TABLE|DELETE\s+FROM|TRUNCATE\s+TABLE)\b", # Destructive SQL Injection
    r"(--|\/\*|\*\/)"                  # SQL Comment injection signs
]

# --- THE FIREWALL MIDDLEWARE LAYER ---
class ApplicationFirewallMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next):
        client_ip = request.client.host
        user_agent = request.headers.get("user-agent", "").lower()
        request_path = str(request.url)

        # 1. Inspect IP Address Authorization
        if client_ip in IP_BLACKLIST:
            print(🚨 FIREWALL BREACH ALERT: Blocked Blacklisted IP {client_ip}")
            return Response(
                content="Access Denied: Your IP address has been flagged by security.",
                status_code=status.HTTP_403_FORBIDDEN
            )

        # 2. Inspect User-Agent Signatures
        for agent_pattern in BANNED_USER_AGENTS:
            if re.search(agent_pattern, user_agent):
                print(f"🚨 FIREWALL REJECTION: Rogue Automated Scanner Detected: '{user_agent}'")
                return Response(
                    content="Malicious User-Agent Blocked.",
                    status_code=status.HTTP_403_FORBIDDEN
                )

        # 3. Inspect URL Path and Query Parameters for Injections (Deep Packet Inspection)
        for pattern in MALICIOUS_PATTERNS:
            if re.search(pattern, request_path):
                print(f"🚨 FIREWALL MALICIOUS PAYLOAD TRIGGERED in URL: IP {client_ip} attempted path manipulation.")
                return Response(
                    content="Security Exception: Malicious payloads detected in request string.",
                    status_code=status.HTTP_400_BAD_REQUEST
                )

        # 4. Inspect Post Bodies (Deep Body Scanning)
        # We perform this selectively or safely to avoid draining memory buffers on massive payloads
        if request.method in ["POST", "PUT", "PATCH"]:
            # Duplicate the body buffer safely so it can still be processed downstream by FastAPI routes
            body_bytes = await request.body()
            body_str = body_bytes.decode("utf-8", errors="ignore")
            
            for pattern in MALICIOUS_PATTERNS:
                if re.search(pattern, body_str):
                    print(f"🚨 FIREWALL INJECTION ATTACK: Blocked POST payload injection attempt from IP {client_ip}")
                    return Response(
                        content="Security Exception: Request body payload rejected.",
                        status_code=status.HTTP_400_BAD_REQUEST
                    )
            
            # Re-inject the body into the request object context so FastAPI can still read it normally
            async def receive():
                return {"type": "http.request", "body": body_bytes, "more_body": False}
            request._receive = receive

        # If all checks pass clean, hand off the request to the next system application controller
        return await call_next(request)

# Register the custom Firewall directly to our application core runtime
app.add_middleware(ApplicationFirewallMiddleware)

# --- STANDARD CONFIGS & APP ENDPOINTS ---
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/api/v1/students")
async def read_students_clean():
    return [{"id": 1, "name": "Alice Vance", "email": "alice@university.edu", "gpa": 3.9}]

@app.post("/api/v1/students")
async def create_student_record(data: dict):
    return {"message": "Data written safely to backend cluster ledger.", "received_payload": data}
