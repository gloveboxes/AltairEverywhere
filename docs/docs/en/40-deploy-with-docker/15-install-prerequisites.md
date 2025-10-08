Install Docker on your computer.

- [Install Docker Desktop on Windows](https://docs.docker.com/desktop/windows/install/)
- [Install Docker Desktop on Mac](https://docs.docker.com/desktop/mac/install/)

- Install Docker on Linux and Raspberry Pi

    Docker has a [script](https://docs.docker.com/engine/install/debian/) to install Docker on Debian, Ubuntu, and Raspberry Pi OS. Run this script using the following command.

    ```bash
     curl -fsSL https://get.docker.com -o get-docker.sh && sudo sh get-docker.sh
    ```

    !!! note "Docker installation notes"

        On arm/v6 (Raspberry Pi Zero and 1), install Docker using:

        ```bash
        sudo apt update
        sudo apt install -y docker.io
        sudo usermod -aG docker $USER
        ```

    Review the [Manage Docker as a non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user) document.

    1. Grant the current user Docker rights

        ```bash
        sudo usermod -aG docker $USER
        ```

    2. Reboot the system.
