# Multi-stage build for distributed HPA* system
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .

# Build master
RUN cd /app/phpa\*/master && \
    cmake . && \
    make

# Build worker
RUN cd /app/phpa\*/worker && \
    cmake . && \
    make

# Runtime image
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled binaries from builder
COPY --from=builder /app/phpa\*/master/build/mphpa /app/master/
COPY --from=builder /app/phpa\*/worker/build/wphpa /app/worker/



# Copy data directory with all maps
COPY data /app/data

# Set environment variables for path resolution
ENV DATA_DIR=/app/data
ENV PATH=/app/master:/app/worker:$PATH

# Set working directory for runtime
WORKDIR /app

# Default to worker (can be overridden: docker run image ./master/mphpa)
CMD ["./worker/wphpa"]
