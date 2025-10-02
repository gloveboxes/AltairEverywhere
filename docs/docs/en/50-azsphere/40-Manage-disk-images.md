# Manage disk images

## Backup an Altair boot SD card

You've been creating apps from the Altair emulator and saving them to the CP/M filesystem. Now you want to backup the contents of the SD card and recreate Altair disk images from the backup.

To create a backup, you extract the Altair SD card contents to a binary file. Then you separate the backup file into individual Altair disk images.

The process is:

1. Remove the SD card from the Azure Sphere device and plug it into your computer.
1. Open a command prompt and navigate to the **Altair_sd_card_image_tools** folder.
1. Power off the Azure Sphere device.
1. Extract the SD card contents to a binary file.

    - On Linux and macOS, run the **DD** command.

        ```bash
        sudo dd if=/dev/sda of=altair_extracted_sd_card_image.bin bs=512 count=12000; sync
        ```

    -  On Windows, download and run [dd](http://www.chrysocome.net/dd).

        List the Windows disks.

        ```powershell
        dd --list
        ```

        The following command assumes the SD card is mounted as the D: drive. Adjust the drive letter to be used based on your Windows disk configuration.

        ```powershell
        dd if=\\.\d: of=altair_extracted_sd_card_image.bin bs=512 count=12000
        ```

    This will read the SD card into a file named *altair_extracted_sd_card_image.bin*.

## Extract Altair disk images from a backup

1. Run the following command to extract Altair disk images from a SD card backup image.

    ```python
    python3 extract_disks_from_sd_card_image.py
    ```

    Using the default settings, the *extract_disks_from_sd_card_image.py* script will create four Altair disk images.

        - azsphere_cpm63k.dsk
        - bdsc-v1.60.dsk
        - escape.dsk
        - blank.dsk

## Create an Altair boot disk image from disk images

Altair disk images can be used to create an Altair boot image to be flashed to an SD card.

To create an SD card binary image from Altair disk images, run the following command:

```bash
create_sd_card_image_from_disks.py
```

By default, the **create_sd_card_image_from_disks.py** Python script will create a binary disk image file named **altair_burn_disk_image.bin** from the following disk images:

    - azsphere_cpm63k.dsk
    - bdsc-v1.60.dsk
    - escape.dsk
    - blank.dsk

