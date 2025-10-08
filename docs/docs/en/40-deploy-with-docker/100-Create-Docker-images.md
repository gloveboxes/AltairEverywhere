## Building Multi-Architecture Docker Images for Altair Everywhere

This guide shows how to build Docker images that work on both x64 (AMD64) and ARM64 architectures using Docker Buildx.

## Prerequisites

- Docker Desktop installed with Buildx support
- Docker Hub account (or another container registry)
- Access to the Altair Everywhere repository

## Step-by-Step Instructions

### 1. Clone the Repository

If you haven't already, clone the Altair Everywhere repository:

```bash
git clone https://github.com/gloveboxes/Altair-8800-Emulator
```

### 2. Navigate to the Docker Directory

```bash
cd Altair-8800-Emulator/Docker
```

### 3. Create and Configure Multi-Architecture Builder

Create a new buildx builder instance that supports multi-platform builds:

```bash
docker buildx create --name multiarch --use
```

Bootstrap the builder to ensure it's ready:

```bash
docker buildx inspect --bootstrap
```

⚠️ **Warning**: This will remove all build cache and may slow down subsequent builds.

### 4. Login to Docker Registry

Make sure you're authenticated with your container registry:

```bash
docker login
```

### 5. Build Multi-Architecture Image

Replace `YOUR_DOCKER_ID` with your actual Docker Hub username:

```bash
docker buildx build . --platform linux/arm64,linux/arm/v7,linux/arm/v6,linux/amd64 --tag YOUR_DOCKER_ID/altair8800:latest --push --no-cache
```

This command will:

- Build for both ARM64, ARMv7, ARMv6, and AMD64 architectures
- Tag the image as `latest`
- Push directly to Docker Hub

### 6. Switch to Default Builder

```bash
docker context use default
```

### Option 2: Remove Custom Builder

```bash
docker buildx rm multiarch
```

This automatically switches back to the default builder.

### Verify Current Builder

Check which builder is currently active:

```bash
docker buildx ls
```

The active builder is marked with an asterisk (*).
