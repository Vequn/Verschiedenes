# ==========================================
# STAGE 1: The Build Environment
# ==========================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install core build chain packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Clone Crow framework repository
RUN git clone https://github.com/CrowCpp/Crow.git

# Copy your local source file into the builder container workspace
COPY core_features.cpp .

# Compile the high-performance C++ backend application with speed optimization flag -O3
RUN g++ -O3 core_features.cpp -I ./Crow/include -lpthread -lboost_system -lsqlite3 -o high_value_engine

# ==========================================
# STAGE 2: The Production Runtime
# ==========================================
FROM ubuntu:22.04

# Install only the lightweight runtime shared library for SQLite3
RUN apt-get update && apt-get install -y libsqlite3-0 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Pull ONLY the compiled binary artifact from Stage 1
COPY --from=builder /build/high_value_engine .

EXPOSE 8000

# Execute the native binary directly
CMD ["./high_value_engine"]
