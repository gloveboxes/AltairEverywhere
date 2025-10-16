FROM debian:trixie-slim

# Install required packages
RUN apt-get update && apt-get install -y \
    libuv1-dev \
    cmake \
    build-essential \
    gdb \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    ca-certificates \
    git \
    libi2c-dev \
    libgpiod-dev \
    gpiod \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspaces/Altair-8800-Emulator

# Set default user
USER root