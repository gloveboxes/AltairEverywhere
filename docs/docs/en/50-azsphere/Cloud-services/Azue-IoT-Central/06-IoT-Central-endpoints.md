---
sidebar_position: 5
---

# Endpoint security

Azure Sphere applications are secure by default, including hardware and network endpoints. You must declare your IoT Central network endpoints, otherwise, your Azure Sphere application will not be able to connect to IoT Central.

Follow these steps to list the network endpoints of your IoT Central application.


1. Open the Command Prompt window.
1. Navigate to the **azure-sphere-samples** folder.

   - **For Windows**: Go to the *Samples\AzureIoT\Tools\win-x64* folder.
   - **For Linux**: Go to the *Samples/AzureIoT/Tools/linux-x64* folder# Azure IoT Central. 
   
      You might need to explicitly set execution permissions for the ShowIoTCentralConfig tool. To do so, from a terminal, run `chmod +x ShowIoTCentralConfig`.

1. Run the ShowIoTCentralConfig tool, and then follow the prompts, as shown in the following table:

   > [!NOTE]
   > Your organization might require consent for the ShowIoTCentralConfig tool to access your Azure IoT Central data in the same way that the Azure API requires such consent.

   | Prompt | Response |
   | --- | --- |
   | **Are you using a legacy (2018) IoT Central application? (Y/N)** | Respond **N**. |
   | **IoT Central App URL** | The URL can be found in your browser address bar (for example, https://apps.azureiotcentral.com). |
   | **API token** | The token can be generated from your Azure IoT Central app. <ol><li>Select **Administration** > **API Tokens** > **Generate Token**.</li> <li>Provide a name for the token (for example, "AzureSphere").</li>  <li>Select **Administrator** as the role, and then select **Generate**.</li>  <li>Copy the token to the clipboard. The token starts with *SharedAccessSignature*. </li> </ol>|
   | **ID Scope** | In the Azure IoT Central application, select **Administration** > **Device Connection**, and then copy the **ID Scope**. |
   | | |

1. Review the output from the ShowIoTCentralConfig tool. It will look similar to the following JSON text:

   ```json
   "CmdArgs": [ "--ScopeID", "0ne0099999D" ],
   "Capabilities": {
      "AllowedConnections": [ "global.azure-devices-provisioning.net", "iotc-9999bc-3305-99ba-885e-6573fc4cf701.azure-devices.net", "iotc-789999fa-8306-4994-b70a-399c46501044.azure-devices.net", "iotc-7a099966-a8c1-4f33-b803-bf29998713787.azure-devices.net", "iotc-97299997-05ab-4988-8142-e299995acdb7.azure-devices.net", "iotc-d099995-7fec-460c-b717-e99999bf4551.azure-devices.net", "iotc-789999dd-3bf5-49d7-9e12-f6999991df8c.azure-devices.net", "iotc-29999917-7344-49e4-9344-5e0cc9999d9b.azure-devices.net", "iotc-99999e59-df2a-41d8-bacd-ebb9999143ab.azure-devices.net", "iotc-c0a9999b-d256-4aaf-aa06-e90e999902b3.azure-devices.net", "iotc-f9199991-ceb1-4f38-9f1c-13199992570e.azure-devices.net" ],
      "DeviceAuthentication": "--- YOUR AZURE SPHERE TENANT ID---",
   }
   ```

1. Copy the output from the ShowIoTCentralConfig tool to Notepad, because you'll use this information in a later unit.
