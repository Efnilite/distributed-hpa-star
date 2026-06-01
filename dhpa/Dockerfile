# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire project (including top-level common/)
COPY ../ .

# Clean old build artifacts
RUN rm -rf dhpa/master/build dhpa/worker/build

# Run the build script from dhpa directory
RUN cd dhpa && bash build.sh

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies (just the C runtime libraries)
RUN apt-get update && apt-get install -y \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built executables from builder
COPY --from=builder /app/dhpa/master/build/master /app/master
COPY --from=builder /app/dhpa/worker/build/worker /app/worker
COPY --from=builder /app/data /app/data

# Make executables runnable
RUN chmod +x /app/master /app/worker

# Default command (can be overridden)
CMD ["/app/master"]
