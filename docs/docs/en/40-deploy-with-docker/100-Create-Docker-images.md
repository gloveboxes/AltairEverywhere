# Build docker images

This page describes how to build the Altair Docker images and is provided for reference only. You don't need to build the Docker images to use the Altair emulator.

## Altair on ARM64 and AMD64

1. Change to the Docker folder

    ```bash
    cd Docker
    ```
    
1. Switch Docker driver

    ```bash
    docker buildx create --use
    ```

1. Clear the Docker Buildx cache

    ```bash
    docker buildx prune -a
    ```

1. Build the Docker image

    ```bash
    docker buildx build . --platform linux/arm64,linux/amd64 --tag YOUR_DOCKER_ID/altair8800:latest --push
    ```

## Altair on Raspberry Pi OS 64 bit (ARM64) with Pi Sense HAT

1. Change to the Docker folder

    ```bash
    cd Docker
    ```

1. Edit and uncomment the following line in the dockerfile to enable PI Sense HAT image build.

    ```text
    #<REMOVE_THIS_COMMENT_TO_ENABLE_PI_SENSE_HAT>RUN cd /Altair8800/AltairHL_emulator && sed -i 's/# set(ALTAIR_FRONT_PI_SENSE_HAT/set(ALTAIR_FRONT_PI_SENSE_HAT/g' CMakeLists.txt
    ```

1. Clear the Docker Buildx cache

    ```bash
    docker buildx prune -a
    ```

1. Build the Docker image

    ```bash
    docker buildx build . --platform linux/arm64 --tag YOUR_DOCKER_ID/altair8800-pisense:latest --push
    ```

## Altair as a shared service

1. Change to the Docker folder

    ```bash
    cd Docker
    ```

1. Edit and uncomment the following line in the dockerfile to enable cloud image build.

    ```text
    #<REMOVE_THIS_COMMENT_TO_ENABLE_CLOUD>RUN cd /Altair8800/AltairHL_emulator && sed -i 's/# set(ALTAIR_CLOUD/set(ALTAIR_CLOUD/g' CMakeLists.txt
    ```

1. Clear the Docker Buildx cache

    ```bash
    docker buildx prune -a
    ```

1. Build the Docker image

    ```bash
    docker buildx build . --platform linux/arm64,linux/amd64 --tag YOUR_DOCKER_ID/altair8800-cloud:latest --push
    ```

## Clean up

After building the Docker images:

1. Clear the Docker Buildx cache

    ```bash
    docker buildx prune -a
    ```

1. Switch back to the default Docker context.

   ```bash
   docker buildx use default && docker context use default 
   ```
