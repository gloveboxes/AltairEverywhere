---
sidebar_position: 4
---

# Azure Sphere trust

Once your IoT Central application has been created, you must enable trust between your Azure Sphere tenant and your Azure IoT Central application. Trust is enabled by sharing your Azure Sphere tenant Certificate Authority certificate with your IoT Central application.

When trust is enabled, any device claimed into your trusted Azure Sphere tenant will be enrolled when it first connects to IoT Central.

Follow these steps to enable trust.

1. Open a **command prompt**.

2. Log in to your Azure Sphere tenant. From the command prompt, run:

   ```azsphere
   azsphere login
   ```

3. Make a note of the current folder. You'll need the name of this folder in the next step.

4. Download the Azure Sphere tenant certificate authority (CA) certificate. From the command prompt, run:

   ```azsphere
   azsphere ca-certificate download --destination CAcertificate.cer
   ```

### Create an Enrollment Group

1. From the IoT Central web portal, select the hamburger button on the top-left corner of the screen to expand the sidebar menu.

1. Select **Administration**, then **Device Connection**.

1. Select **+ New**.

1. Name the enrollment group **Azure Sphere**.
1. Leave group type set to **IoT devices**.
1. Select Certificates (X.509) from the Attestation type dropdown.
1. Select **Save**.

### Upload the Azure Sphere tenant CA certificate to Azure IoT Central

1. Select **+ Manage primary**.

1. Select the folder icon next to the **Primary** box and navigate to the folder where you downloaded the certificate. If you don't see the .cer file in the list, set the view filter to **All files (*)**. Select the certificate and then select the gear icon next to the **Primary** box.

1. The **Primary Certificate** dialog box appears. The **Subject** and **Thumbprint** fields contain information about the current Azure Sphere tenant and primary root certificate.

### Verify the tenant CA certificate

1. Select the **Generate verification code**.
1. Copy the verification code to the clipboard.

    ![Screenshot that shows how to verify a certificate.](img/verify-certificate.png)

1. Download a validation certificate that proves you own the Azure Sphere tenant CA certificate. Replace **<code\>** in the command with the verification code you copied to the clipboard. From a command prompt, run:

   ```azsphere
   azsphere ca-certificate download-proof --destination ValidationCertification.cer --verification-code <code>
   ```

    The Azure Sphere Security Service signs the validation certificate with the verification code to prove that you own the Certificate Authority (CA).

<!-- ### Verify the tenant's identity -->

1. Return to Azure IoT Central and select **Verify**.

1. When prompted, select the validation certificate that you generated in the previous step. When the verification process is complete, the **Primary Certificate** dialog box displays the **Verified** message.

1. Select **Close** to dismiss the box.
1. Select **Save**.

    ![Screenshot that shows a verified certificate.](img/certificate-verified.png)

---
