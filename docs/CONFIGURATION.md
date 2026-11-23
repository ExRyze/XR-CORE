# Configuration and security

## Firmware defaults

Edit `Config.cpp` for device identity and defaults. `Config.h` documents feature declarations and the default SX1276 wiring. Keep real secrets out of source control and avoid copying them into documentation or screenshots.

| Setting | Purpose |
| --- | --- |
| Device brand, name, version | OLED labels and startup diagnostics. |
| `AP_SSID` | Name of the device's setup access point. |
| `AP_PASS` | Access Point password mode: `nullptr` for open mode, `""` to generate and persist a random password, or an 8–63 character string to use directly. |
| `WEB_AUTH_USERNAME`, `WEB_AUTH_PASSWORD` | Dashboard login credentials. Replace the checked-in password with a unique value before deployment. |
| `MQTT_*` | Default broker, port, client ID, and credentials. Stored web settings may override defaults. |
| `LORA_ENABLED` and `LORA_*` | Radio default and GPIO/radio profile. Stored web settings may override feature settings. |
| `ARDUINO_CLOUD_ENABLED`, `CLOUD_DEVICE_ID`, `CLOUD_DEVICE_KEY` | Arduino IoT Cloud feature flag and Thing credentials. |

The configuration currently defines `MQTT_ENABLED`, `LORA_ENABLED`, and `ARDUINO_CLOUD_ENABLED` as false. Enable and configure only integrations that are provisioned and available on the target hardware.

## Wi-Fi provisioning

On startup, the Wi-Fi manager loads saved station credentials. A GPIO 0 button is configured with the internal pull-up and is used by the manager's AP-mode handling. If station connection does not succeed before the configured timeout, the manager starts its access point. Use the displayed/access point details and dashboard Wi-Fi settings to configure the device. The exact button gesture and AP password behavior are implemented in `WiFi_Manager.cpp` and should be checked against the target board before deployment.

Credentials are stored in ESP32 Preferences. Clearing Wi-Fi credentials returns the device to provisioning behavior. Treat the setup AP as a trusted local interface.

## MQTT and TLS

MQTT options are persisted in Preferences and exposed through authenticated web settings. TLS can use a CA certificate stored at `/mqtt-ca.crt` in LittleFS; the web configuration flow supports uploading a certificate up to 8 KB. Provide the correct broker CA and hostname, and keep certificate verification enabled. A blank or invalid CA can prevent a TLS connection.

The firmware defaults include an example broker host and port, which must be replaced with reachable values. Never treat those placeholders as a production configuration. Filesystem re-upload can erase the uploaded CA certificate.

## LoRa

The default hardware profile is SX1276 with NSS GPIO 5, reset GPIO 14, DIO0 GPIO 2, and DIO1 `-1`. Verify those pins and frequency rules for the exact board/module and deployment location. LoRa settings include frequency, bandwidth, spreading factor, coding rate, TX power, sync word, preamble, CRC, and node ID. Enabling the radio does not by itself define a packet format or application-level delivery behavior.

## Arduino IoT Cloud

Create/provision the relevant Arduino IoT Cloud Thing and put its device ID and secret key in local configuration. Keep those values private. Cloud integration is disabled by default.

## Web access

The dashboard login is enforced by a firmware session cookie for protected API routes. The interface uses plain HTTP; the cookie/session does not encrypt traffic. Use a trusted LAN, change the supplied login password, and do not forward the web server from a public router. The public device status endpoint is intentionally unauthenticated and should not be assumed to contain private information.

## Secret handling checklist

- Replace the checked-in web password and any default AP credentials before deployment.
- Do not commit Wi-Fi, broker, Cloud, CA private keys, or other device secrets.
- Use unique credentials per deployment and rotate any credential that was exposed.
- Keep TLS certificate verification enabled for MQTT.
- Restrict access to the device network and protect filesystem images that contain credentials or certificates.
