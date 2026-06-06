import time
import logging
from typing import Final
from fastapi import FastAPI, Request, Response, status
from fastapi.responses import JSONResponse
from fastapi.exceptions import RequestValidationError
from pydantic import BaseModel, Field, field_validator

# Import the core verification system designed previously
from validator import AdvancedCreditCardValidator

# ==========================================
# SYSTEM SETUP & LOGGING CONFIGURATION
# ==========================================
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("EnterpriseAPI")

app = FastAPI(
    title="Financial Token Validation Service",
    version="1.0.0",
    description="High-performance asynchronous API for structural card validation.",
    docs_url="/api/v1/docs",
    redoc_url="/api/v1/redoc"
)

# Instantiate stateless validation engine core
VALIDATOR_ENGINE: Final[AdvancedCreditCardValidator] = AdvancedCreditCardValidator()

# Hard runtime configuration constants
MAX_PAYLOAD_BYTES: Final[int] = 1024  # Avoid DoS attacks by restricting payload to 1KB


# ==========================================
# DATA INGESTION LAYERS (OPTIMIZED SCHEMAS)
# ==========================================
class CardValidationRequest(BaseModel):
    """Data Validation model with explicit runtime validation bounds."""
    card_number: str = Field(
        ..., 
        min_length=12, 
        max_length=30, 
        description="Raw credit card or transactional string format."
    )

    @field_validator("card_number")
    @classmethod
    def check_non_empty_whitespace(cls, value: str) -> str:
        """Pydantic level baseline check to block completely empty strings early."""
        if not value.strip():
            raise ValueError("Input sequence cannot consist solely of whitespace characters.")
        return value


# ==========================================
# ENTERPRISE INTERCEPTOR MIDDLEWARES
# ==========================================
@app.middleware("http")
async def security_and_telemetry_middleware(request: Request, call_next):
    """
    High-performance middleware wrapper layer tracking process time 
    and enforcing content-length payload limits.
    """
    start_time = time.perf_counter()

    # 1. Protect memory allocations against infinite stream byte overflows
    content_length = request.headers.get("content-length")
    if content_length and int(content_length) > MAX_PAYLOAD_BYTES:
        return JSONResponse(
            status_code=status.HTTP_413_REQUEST_ENTITY_TOO_LARGE,
            content={"success": False, "error": "Payload size limit breached. Maximum 1KB allowed."}
        )

    # 2. Forward request downstream through asynchronous cycle
    response: Response = await call_next(request)

    # 3. Process execution telemetry calculations
    process_time = (time.perf_counter() - start_time) * 1000  # Convert to milliseconds
    response.headers["X-Process-Time-Ms"] = f"{process_time:.4f}"
    
    logger.info(f"Route: {request.url.path} | Method: {request.method} | Telemetry Duration: {process_time:.4f}ms")
    return response


# ==========================================
# GLOBAL ERROR STANDARD DISPATCHERS
# ==========================================
@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exc: RequestValidationError):
    """Overrides default validation errors to avoid leaking system stack properties."""
    return JSONResponse(
        status_code=status.HTTP_422_UNPROCESSABLE_ENTITY,
        content={
            "success": False,
            "error": "Malformed structural input payload constraint violations.",
            "details": exc.errors()
        }
    )

@app.exception_handler(Exception)
async def generic_internal_exception_handler(request: Request, exc: Exception):
    """Catch-all top-level fail-safe to suppress unhandled execution crashes."""
    logger.critical(f"Unhandled catastrophic system exception captured: {str(exc)}", exc_info=True)
    return JSONResponse(
        status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
        content={
            "success": False,
            "error": "An internal service isolation failure occurred. Secure fallback initialized."
        }
    )


# ==========================================
# PRODUCTION API ROUTE DEF
# ==========================================
@app.post("/api/v1/validate", status_code=status.HTTP_200_OK)
async def process_card_verification(payload: CardValidationRequest):
    """
    Stateless high-concurrency evaluation endpoint executing non-blocking verification loops.
    """
    # High-performance validation computation execution
    telemetry = VALIDATOR_ENGINE.validate(payload.card_number)

    # Clean pipeline structured response execution mapping
    return {
        "success": True,
        "data": {
            "isValid": telemetry.is_valid,
            "network": telemetry.issuer_name,
            "telemetry": {
                "luhnChecksumPassed": telemetry.luhn_passed,
                "lengthConstraintPassed": telemetry.length_passed,
                "extractedLength": telemetry.sanitized_length
            }
        }
    }
