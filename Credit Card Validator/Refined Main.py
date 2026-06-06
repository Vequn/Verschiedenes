from fastapi import FastAPI, Request, status
from prometheus_client import Counter, Histogram, generate_latest, CONTENT_TYPE_LATEST
from fastapi.responses import Response
from validator import AdvancedCreditCardValidator

app = FastAPI(title="Production Card Validation Engine")
VALIDATOR_ENGINE = AdvancedCreditCardValidator()

# ==========================================
# PROMETHEUS METRIC DEFINITIONS
# ==========================================
REQUEST_COUNT = Counter(
    "http_requests_total", 
    "Total number of HTTP requests processed", 
    ["method", "endpoint", "http_status"]
)

REQUEST_LATENCY = Histogram(
    "http_request_duration_seconds", 
    "Time spent processing HTTP requests", 
    ["endpoint"]
)

VALIDATION_VERDICTS = Counter(
    "card_validation_verdicts_total",
    "Tracking successful vs malicious card entry results",
    ["issuer", "is_valid"]
)

# ==========================================
# METRICS CAPTURE MIDDLEWARE
# ==========================================
@app.middleware("http")
async def monitor_telemetry_layer(request: Request, call_next):
    # Ignore tracking metrics for the tracking endpoint itself to avoid loops
    if request.url.path == "/metrics":
        return await call_next(request)

    # Start timer gauge
    with REQUEST_LATENCY.labels(endpoint=request.url.path).time():
        response = await call_next(request)
        
        # Increment total request counter with response metadata labels
        REQUEST_COUNT.labels(
            method=request.method, 
            endpoint=request.url.path, 
            http_status=response.status_code
        ).inc()
        
        return response

# ==========================================
# METRICS SCRAPE ENDPOINT
# ==========================================
@app.get("/metrics")
def metrics_endpoint():
    """Private monitoring hook where Prometheus pulls execution logs."""
    return Response(content=generate_latest(), media_type=CONTENT_TYPE_LATEST)


# ==========================================
# CORE VALIDATION WITH METRIC COUPLING
# ==========================================
@app.post("/api/v1/validate")
async def process_card_verification(payload: dict):
    # Run the core validation pipeline
    telemetry = VALIDATOR_ENGINE.validate(payload.get("card_number", ""))

    # Track domain-specific outcomes in real-time
    VALIDATION_VERDICTS.labels(
        issuer=telemetry.issuer_name,
        is_valid=str(telemetry.is_valid)
    ).inc()

    return {"success": True, "isValid": telemetry.is_valid, "network": telemetry.issuer_name}
