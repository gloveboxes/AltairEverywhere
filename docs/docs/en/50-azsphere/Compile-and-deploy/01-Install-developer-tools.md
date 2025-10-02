---
sidebar_position: 1
---

# Install developer tools

## Troubleshooting

1. If you have trouble connecting to the Azure Sphere over USB be sure to disable any VPNs you might have enabled.
2. The **TAP-Windows Adapter V9** installed with VPN clients, including the OpenVPN client is not compatible with the **TAP-Windows Adapter V9** required and installed by the Azure Sphere SDK. You will need to uninstall the VPN client and reinstall the Azure Sphere SDK for Visual Studio.
3. Windows Users. If running the IoT Central ShowIoTCentralConfig command fails with a missing library message then delete the folder from ShowIoTCentralConfig from AppData\\Local\\Temp\\.net.

---

## Introduction

Follow the [Windows](#windows-users) or [Ubuntu](#ubuntu-users) Quickstarts linked below to install the developer tools required to build and deploy the *Predictive Maintenance*.

---

## Windows users

Azure Sphere development is supported on Windows 10 and 11.

You need to complete these steps:

1. Install the latest Azure Sphere SDK.
1. Install CMake and Ninja.
1. Install Visual Studio Code.
1. Install the Visual Studio Code Azure Sphere extension.
1. Claim your device.
1. Configure networking for the device.

The [Quickstart: Install the Azure Sphere SDK for Windows](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk?pivots=visual-studio) will step you through the process.

### Install the Git client for Windows

The Git client is required to clone the *Predictive Maintenance* solution to your computer.

Install [Git for Windows](https://git-scm.com/downloads?azure-portal=true).

### Install the GNU Arm Embedded Toolchain for Windows

1. Download the [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads?azure-portal=true) for Windows.
2. Install the GNU Arm Embedded Toolchain

---

## Ubuntu users

Azure Sphere development is supported on Ubuntu 20.04 and 22.04 LTS.

You need to do the following:

1. Install the Azure Sphere SDK.
1. Set up the device connection.
1. Install CMake and Ninja.
1. Install Visual Studio Code.
1. Install the Visual Studio Code Azure Sphere extension.
1. Claim your device.
1. Configure networking for the device.

The [Quickstart: Install the Azure Sphere SDK for Linux](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk-linux?pivots=vs-code-linux) will step you through the process.

### Install the Git client for Linux

The Git client is required to clone the *Predictive Maintenance* solution to your computer.

```bash
sudo apt install git
```

### Install the GNU Arm Embedded Toolchain for Linux

Install the GNU Arm Embedded Toolchain for Linux

1. Download the latest [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads?azure-portal=true).
2. Install the downloaded package. The following command installs the toolchain in the /opt directory. Note, you will need to update the filename to match the version you downloaded.

    ```bash
    sudo tar -xjvf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt
    ```

3. Update your path. Open ~/.bashrc and add to the end.

    ```bash
    export PATH=$PATH:/opt/gcc-arm-none-eabi-10.3-2021.10/bin
    ```

4. Optional: The real-time core debugger relies on the *libncurses.so.5* library. Depending on your system setup, this library may already be install, if not, then run the following commands.

    ```bash
    sudo add-apt-repository universe
    sudo apt-get install libncurses5
    ```

---

## Recommended Visual Studio Code Extension

The Peacock extension allows you to change the color of your Visual Studio Code workspace. The Peacock extension is useful when you have multiple instances of Visual Studio Code open debugging code on high-level and real-time cores.

   1. Open Extensions sideBar panel in Visual Studio Code
      - Or choose the menu options for View → Extensions
   1. Search for Peacock
   1. Click Install
   1. Click Reload, if required

---

## Delete existing applications on Azure Sphere

1. From the Windows **PowerShell command-line** or Linux **Terminal**, run the following command to delete any existing applications on the device.

   ```
   azsphere device sideload delete
   ```

2. Restart Azure Sphere.

   ```
   azsphere device restart
   ```

---

## Enable high-level core development

1. From the Windows **PowerShell command-line** or Linux **Terminal**, run the following command to enable high-level app development on the device.

   ```
   azsphere device enable-development
   ```

---

## Enable real-time core development

### Windows users

1. Open the Windows **PowerShell command-line** as **Administrator**, and run the following command to enable real-time core development on the device.

   ```
   azsphere device enable-development -r
   ```

2. Close the Windows **PowerShell command-line**

### Ubuntu users

1. Open the Linux **Terminal** and run the following command to enable real-time core development on the device.

   ```bash
   azsphere device enable-development -r
   ```
