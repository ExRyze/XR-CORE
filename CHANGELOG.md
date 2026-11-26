# Changelog

## Unreleased

- Added first-boot Wi-Fi default migration into NVS, with saved NVS credentials taking precedence and AP fallback when no credentials are configured.
- Added configurable AP captive-portal behavior through `WEB_AP_AUTO_OPEN` and documented direct-IP fallback.
- Isolated MQTT servicing on a low-priority core 1 task, set TLS as the default for the configured TLS broker, and kept the configured enabled state separate from client readiness.
- Documented firmware modules, web UI workflow, hardware defaults, configuration, and build/upload procedures.
- Recorded the supplied ESP32 Arduino core and library versions and compile-size reference.
- Added repository guidance for provisioning, LittleFS assets, and optional integrations.
