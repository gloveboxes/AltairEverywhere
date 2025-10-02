# Deployment options

There are three ways to deploy the Altair 8800 emulator.

1. [Deploy with Docker](../40-deploy-with-docker/10-introduction.md)
2. [Build from source](../45-build-from-source/01-Introduction.md)
3. [Deploy on Azure Sphere](../50-azsphere/01-Introduction.md)

## Deploying the Altair 8800 with Docker

The **fastest** and **easiest** way to get started with the Altair emulator is on a system running Docker. 

You can run Docker on 64-bit versions of Linux, macOS, Windows, and Raspberry Pi OS. There are 64-bit Docker images for Linux, macOS, Windows, ChromeOS, and Raspberry Pi OS. 

There is also a separate Raspberry Pi OS image for a Raspberry Pi with a Pi Sense HAT.

For more information, refer to [Deploy with Docker](../40-deploy-with-docker/10-introduction.md).

## Build from source

You can also build the Altair emulator from the source code and run it as a native application on your computer. For more information, refer to [Compiling the Altair emulator](../45-build-from-source/01-Introduction.md).

Reasons to do this include:

1. You want to make changes to the Altair emulator code.
2. You want to run the Altair emulator on a device that does not support Docker.
3. You'd want to learn how to build the Altair emulator from the source code.
4. You have an Apple Silicon Mac and want to run the Altair emulator natively on one of the Apple Silicon efficiency cores.

Building the Altair 8800 emulator from source has been tested and is supported on the following POSIX compatible operating systems.

- Windows 11 with [WSL 2](https://docs.microsoft.com/windows/wsl/)
- Ubuntu 20.04 and 22.04
- macOS,
- Raspberry Pi OS 64-bit

## Azure Sphere

If you have an [Azure Sphere](https://azure.microsoft.com/services/azure-sphere/) device, you can run the Altair 8800 emulator on it. The Altair emulator runs as a native application on the Azure Sphere device. The Altair emulator is a [POSIX](https://en.wikipedia.org/wiki/POSIX) compatible application and runs on the Azure Sphere OS.

Follow the [Deploy on Azure Sphere](../50-azsphere/01-Introduction.md) instructions.
