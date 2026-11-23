# Security policy

## Reporting

Do not publish device credentials, Wi-Fi passwords, MQTT secrets, Arduino IoT Cloud keys, or private certificate material in issues or pull requests. Report security concerns privately to the project maintainer.

## Deployment guidance

- Replace the example web login password before deployment.
- Use unique credentials and rotate secrets that have been exposed.
- Keep the device web interface on a trusted local network. It uses HTTP and should not be exposed directly to the public internet.
- Provide the correct MQTT CA certificate for TLS and leave certificate verification enabled.
- Treat LittleFS images and firmware binaries as potentially sensitive when they contain certificates or configuration data.

## Known constraints

The web UI runs over unencrypted HTTP. Session authentication protects dashboard APIs from unauthenticated browser requests but does not encrypt traffic or provide protection from a hostile network operator. Plan network isolation accordingly.
