#pragma once

// Copy this file to include/secrets.h and fill in your own values.
// Never commit include/secrets.h.

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Use the IoT Hub name only, for example: guardian-tone-hub
#define AZURE_IOT_HUB_NAME "YOUR_IOT_HUB_NAME"
#define AZURE_DEVICE_ID "YOUR_DEVICE_ID"

// Paste a DEVICE-scoped SAS token here. It should begin with
// "SharedAccessSignature ...". Generate it for the device identity rather
// than using the iothubowner service policy.
#define AZURE_SAS_TOKEN "SharedAccessSignature sr=..."

// Optional server root CA PEM. If left empty, the prototype falls back to
// insecure TLS so a class demo can still be brought up quickly. For a final
// deployment, paste the Azure-trusted root CA here and do not use insecure TLS.
#define AZURE_ROOT_CA ""
