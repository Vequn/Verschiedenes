# ==========================================
# STAGE 1: Build Dependencies
# ==========================================
FROM python:3.11-slim AS builder

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends gcc && \
    rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
# Install to an alternate user path to simplify copying into stage 2
RUN pip install --no-cache-dir --user -r requirements.txt

# ==========================================
# STAGE 2: High-Performance Runtime Environment
# ==========================================
FROM python:3.11-slim AS runtime

WORKDIR /app

# Copy installed system site-packages from build layer
COPY --from=builder /root/.local /root/.local
COPY . /app

ENV PATH=/root/.local/bin:$PATH
ENV PYTHONUNBUFFERED=1

# Expose production-layer internal binding port
EXPOSE 8000

# Security Invariant: Drop root privileges to mitigate execution threat profiles
USER 10001

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD ["python", "-c", "import urllib.request; urllib.request.urlopen('http://localhost:8000/api/v1/docs', timeout=2)"]

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "4", "--log-level", "warning"]
