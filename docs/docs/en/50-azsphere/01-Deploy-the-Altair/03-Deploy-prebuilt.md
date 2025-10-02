---
sidebar_position: 3
---

# Deploy pre-built images

Install the following real-time core app services for the Altair Emulator.

1. The movement classification service.
1. The storage service.



## Deploy the movement classification service

1. Open a command prompt and navigate to the **Altair_imagepackages** folder.
1. From the command prompt, run the following command to deploy the movement classification service.

    ```bash
    azsphere device sideload deploy -p AltairRT_movement_classify.imagepackage
    ```

## Deploy the SD card service

If you the Mikroe SD card Click then deploy the SD card service. Ensure the MikroE SD card Click is plugged in Click Socket 1 on the Avnet Azure Sphere board. If you don't have an SD card Click then deploy the differencing disk service.

Select the imagepackage that matches your Avnet Azure Sphere Starter Kit.

| Device  | Image Package Name   |
|---|---|
| Avnet Rev 1 |  AltairRT_sd_card_service_avnet_rev_1.imagepackage  |
| Avnet Rev 2 |  AltairRT_sd_card_service_avnet_rev_2.imagepackage  |

1. Open a command prompt and navigate to the **Altair_imagepackages** folder.
1. From the command prompt, run the following command to deploy the SD Card service.

    ```bash
    azsphere device sideload deploy -p IMAGE_PACKAGE_NAME.imagepackage
    ```

## Deploy the differencing disk service

If you don't have an SD card Click then deploy the differencing disk service. 

The differencing disk services provides approximately 100 KB of temporary read/write storage for the Altair filesystem. Any changes made to the Altair filesystem will be lost when the power to the device is cycled.

1. Open a command prompt and navigate to the **Altair_imagepackages** folder.
1. From the command prompt, run the following command to deploy the differencing disk.

    ```bash
    azsphere device sideload deploy -p AltairRT_difference_disk_service.imagepackage
    ```

## Deploy the Altair emulator

There are a number of prebuilt Altair emulator images. If your configuration matches one from the list then deploy that image. Failing that, you will need to compile the Altair emulator for your specific configuration.

Select the imagepackage that matches your configuration.

| Device  | Image Package Name   |
|---|---|
| Avnet Rev 1 with SD card and 8800 Retro Click | emulator_avnet_r1_sd_retro.imagepackage |
| Avnet Rev 2 with SD card and 8800 Retro Click | emulator_avnet_r2_sd_retro.imagepackage |

1. Open a command prompt and navigate to the **Altair_imagepackages** folder.
1. From the command prompt, run the following command to deploy the SD Card service.

    ```bash
    azsphere device sideload deploy -p IMAGE_PACKAGE_NAME.imagepackage
    ```
