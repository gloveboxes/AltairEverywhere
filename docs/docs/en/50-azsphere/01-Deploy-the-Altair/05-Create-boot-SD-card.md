---
sidebar_position: 5
---

# Create a boot SD card

Follow these instructions to create an Altair boot SD card or a Wi-Fi enabled Altair boot SD card.

## Install the prerequisite software

1. Install [Balena Etcher](https://www.balena.io/etcher) to flash an SD card.
1. Install Python 3 from the [Python download web page](https://www.python.org/downloads).
1. Install [PyYaml](https://pypi.org/project/PyYAML/)

## Create an Altair boot SD card

1. Plug the Micro SD Card into your computer.
1. Start Balena Etcher.
1. Select **Flash from file**.
1. Select the **altair_burn_disk_image.bin** from the **Altair_sd_card_image_tools** folder.
    > Note, when warned the partition table is missing, select **Continue**.
1. Select the target.
1. Select Flash to write the contents of the altair_burn_disk_image.bin file to the SD card.
1. When the copy has completed, remove the SD card from your computer, insert into the Mikroe micro SD card Click, then power on the Azure Sphere device.

## Create a Wi-Fi enabled Altair boot SD card

The Azure Sphere supports the following Wi-Fi network authentication protocols.

- Open
- PSK
- EAP-TLS

### Create a Wi-Fi profile

1. From a command prompt, navigate to the **Altair_sd_card_image_tools** folder.
1. There are three Wi-Fi profiles in this folder. Select and edit the profile based on your Wi-Fi authentication requirements.

    - wifi_open.yaml
    - wifi_psk.yaml
    - wifi_eap.yaml

### Create a Wi-Fi enabled Altair boot image

The following command adds the Wi-Fi profile to the  **altair_burn_disk_image.bin** boot image and generates the **altair_burn_disk_image_wifi.bin** boot image.

1. Run the **altair_wifi_config.py** script passing the desired Wi-Fi profile.

    ```bash
    python3 altair_wifi_config.py -p {profile}.yaml
    ```

### Flash the Wi-Fi enabled Altair boot image
 
Finally, flash the Altair boot image to an SD card. When the Altair emulator boots from the SD card it will look for a Wi-Fi profile. If a Wi-Fi profile is found, the Azure Sphere Wi-Fi network is provisioned and then the Wi-Fi profile is erased from the SD card.

1. Plug the Micro SD Card into your computer.
1. Start Balena Etcher.
1. Select **Flash from file**.
1. Select the **altair_burn_disk_image_wifi.bin** from the **Altair_sd_card_image_tools** folder.
    > Note, when warned the partition table is missing, select **Continue**.
1. Select the target.
1. Select Flash to write the contents of the **altair_burn_disk_image_wifi.bin** file to the SD card.
1. When the copy has completed, remove the SD card from your computer, insert into the Mikroe micro SD card Click, then power on the Azure Sphere device.

