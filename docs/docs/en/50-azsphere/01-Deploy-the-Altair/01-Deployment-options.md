---
sidebar_position: 1
---

# Deployment options

The easiest way to deploy the Altair emulator the Azure Sphere is to use prebuilt images. To integrate cloud services then you will need to install the Azure Sphere developer tools and create the cloud services you want to use.

## Ideal Azure Sphere config

The ideal Azure Sphere configuration is the Avnet Azure Sphere Starter Kit paired with the [MikroE microSD Click](https://www.mikroe.com/microsd-click) and the [MikroE 8800 Retro Click](https://www.mikroe.com/8800-retro-click).

## Supported configurations

There are two storage options.

| Option | Pros   | Cons |
|---|---|---|
| Azure Sphere real-time difference disk service.  | No extra hardware required.  | Disk changes are lost if the device is reset or power is cycled. |
| [MikroE microSD Click](https://www.mikroe.com/microsd-click). Requires an Avnet Azure Sphere Start Kit Rev 1 or 2. | Permanent storage on SD card. | Additional hardware required. |

There are three display options.

| Option | Pros   | Cons |
|---|---|---|
| No display.  | No extra hardware required. | No visual indication of the state of the Altair. |
| [MikroE 8800 Retro Click](https://www.mikroe.com/8800-retro-click). | Display the state of the Altair, display characters and bitmaps. | Additional hardware required. |
| The [Altair Front Panel Kit](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800.Hardware). | Authentic Altair 8800 experience. | Requires skills and equipment to make the front panel kit. **Can't** be used with the [MikroE 8800 Retro Click](https://www.mikroe.com/8800-retro-click). |

