---
sidebar_position: 2
---

# Deploy the Altair emulator

## CMakeList configuration

Open the **AltairHL_emulator** folder with Visual Studio Code.

1. Update the CMakeList.txt configuration file

    The CMakeList.txt file must be configured to match your hardware. The following snippet of CMakeList shows the available configuration options.

    ```cmake
    # SELECT DEVELOPER BOARD ##########################################################################################
    #
    set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1 ")
    # set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2 ")
    # set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or rdb)")
    # set(SEEED_STUDIO_RDB_ETHERNET TRUE "Seeed Studio Azure Sphere MT3620 Development Kit with ethernet shield")
    # set(SEEED_STUDIO_MDB TRUE "Seeed Studio Azure Sphere Mini Developer Board")
    #
    ###################################################################################################################

    # SELECT FRONT PANEL CONFIG #######################################################################################
    #
    set(ALTAIR_FRONT_PANEL_NONE TRUE "Altair on Azure Sphere with no panel.")
    # set(ALTAIR_FRONT_PANEL_RETRO_CLICK TRUE "Avnet with the MikroE 8800 Retro Click")
    # set(ALTAIR_FRONT_PANEL_KIT TRUE "Altair front panel board")
    #
    ###################################################################################################################

    # ENABLE SD CARD ##################################################################################################
    #
    # The MikroE microSD Click works with the Avnet Azure Sphere Starter Kit Rev 1 and Rev 2 when in Socket 1.
    # The MikroE microSD Click works with the Mikroe 8800 Retro Click in Socket 2
    # Uncomment the "set(MICRO_SD_CLICK" line below to enable the MikroE microSD Click
    #
    # set(MICRO_SD_CLICK TRUE "MikroE microSD Click for CP/M read/write to SD Card")
    #
    ###################################################################################################################
    ```


### Enable Front Panel kit

If you have built an Altair Front Panel, you need to enable Azure Sphere SPI on ISU1.  Open the **app_manifest.json** file and uncomment the following line.

```text
//  Uncomment the following line to enable SPI on ISU1 for the Altair Front Panel
// "SpiMaster": [ "$MT3620_ISU1_SPI" ],
```

## Deploy the Altair emulator to the Azure Sphere.

1. Ensure **Release** configuration is selected.
1. Press <kbd>ctrl+F5</kbd> to compile and deploy the Altair emulation application to your Azure Sphere.

## Connect the Altair web terminal

The Altair emulator will start running on the Azure Sphere device. Now connect the [Altair web terminal](../../20-fundamentals/25-Web-Terminal.md).
