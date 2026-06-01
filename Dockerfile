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

# # Build master
# RUN cd /app/dhpa/master && \
#     rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake && \
#     cmake -B build && \
#     make -C build -j && \
#     cp /app/dhpa/master/build/master /app/master

# # Build worker
# RUN cd /app/dhpa/worker && \
#     rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake && \
#     cmake -B build && \
#     make -C build -j && \
#     cp /app/dhpa/worker/build/worker /app/worker

COPY dhpa/master/build/master /app/master
COPY dhpa/worker/build/worker /app/worker

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy data directory with all maps
COPY data /app/data

# Set environment variables for path resolution
ENV DATA_DIR=/app/data
ENV PATH=/app/master:/app/worker:$PATH

# Set working directory for runtime
WORKDIR /app
