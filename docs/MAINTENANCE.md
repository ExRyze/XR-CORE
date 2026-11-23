# Maintenance

## Updating the frontend

1. Edit the source under `local web/`.
2. Run `npm run build` from that directory.
3. Confirm generated HTML references the newly generated bundles in `data/assets/`.
4. Preserve `data/logo.raw` and any required filesystem runtime files.
5. Upload the LittleFS image using the same partition profile as the firmware.

## Updating the OLED logo

Replace `data/logo.png`, run `python tools/convert_logo.py` from the project root, inspect `tools/logo_preview.png`, and upload the updated filesystem image. The converter assumes the source image has transparent or black background pixels and emits a fixed 54 x 54 bitmap.

## Updating firmware dependencies

Record the ESP32 board package and third-party library versions used for a successful build. Recompile after changing a library or core version, and check web server callbacks, RadioLib APIs, Arduino IoT Cloud APIs, and filesystem behavior for compatibility.

## Operational checks

- Check serial output at 115200 baud after a firmware upload.
- Confirm the OLED initializes and cycles through its pages.
- Provision Wi-Fi and verify station/AP behavior.
- Log into the dashboard and check the device status and telemetry endpoint.
- If enabled, validate MQTT broker reachability and TLS certificate validation.
- If enabled, validate LoRa wiring and radio initialization with the intended region settings.
- Confirm the filesystem image contains the dashboard and `logo.raw` after upload.

## Limitations

These checks are a maintenance guide, not evidence that every board, peripheral, network, or optional cloud integration has been tested. The compile sizes in the README come from one supplied Arduino build log.
