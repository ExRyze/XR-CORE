# XR-CORE MQTT Topic and Payload Specification

The default base topic is `exryze/xrcore/001`. The base topic can be changed in the local MQTT settings; an empty value uses the default.

| Topic | Purpose | Publish QoS | Retain | Frequency |
|---|---|---:|---|---|
| `exryze/xrcore/001/telemetry` | Sensor measurements | 0* | No | Every 5 seconds (`MQTT_TELEMETRY_INTERVAL_MS`) |
| `exryze/xrcore/001/status` | Presence and runtime health | 0* | Yes | On connect, then every 60 seconds |
| `exryze/xrcore/001/info` | Device and firmware metadata | 0* | Yes | Once per boot after a successful publish |

The Last Will and Testament is registered on the status topic with QoS 1 and retain enabled. The current firmware uses PubSubClient 2.8, whose publish API only supports QoS 0; `*` marks that implementation limitation. Publish QoS 1 requires replacing/upgrading the MQTT client library and validating its TLS, subscription, and callback behavior.

All timestamps are Unix seconds in UTC. When NTP has not synchronized, `ts` is `0` and `timeSynced` is `false`. Payloads are compact JSON without comments or trailing commas.

## Telemetry

The telemetry payload mirrors the validated package and contains raw and filtered BME680 temperature. `ts` is UTC publish time when NTP is available; `timestampMs` is the ESP32 monotonic time of the sensor sample.

```json
{
  "schemaVersion": 1,
  "deviceId": "XR-CORE-01",
  "seq": 1042,
  "ts": 1795667400,
  "timeSynced": true,
  "source": "bme680",
  "measurement": "temperature",
  "rawValue": 24.7,
  "filteredValue": 24.5,
  "unit": "C",
  "filter": "MOVING_AVERAGE",
  "initialized": true,
  "online": true,
  "valid": true,
  "timestampMs": 42000
}
```

The `source`, `measurement`, `rawValue`, `filteredValue`, `unit`, `filter`, lifecycle, validity, and sample timestamp fields mirror the validated telemetry package. `seq` increments for each telemetry publish attempt. The sample payload is not retained. No telemetry message is sent until the package is valid.

## Status

### Last Will and Testament

The broker publishes this retained message if the client disconnects unexpectedly:

```json
{
  "schemaVersion": 1,
  "deviceId": "XR-CORE-01",
  "state": "offline"
}
```

### Online message

The device publishes this retained message after connecting and every 60 seconds while connected:

```json
{
  "schemaVersion": 1,
  "deviceId": "XR-CORE-01",
  "state": "online",
  "ts": 1795667400,
  "timeSynced": true,
  "uptime": 86400,
  "wifiRssi": -58,
  "loraReady": true,
  "freeHeap": 187432,
  "minFreeHeap": 156320
}
```

## Info

Published as a retained message once per boot, after MQTT connects. If publishing fails, firmware retries while connected.

```json
{
  "schemaVersion": 1,
  "deviceId": "XR-CORE-01",
  "ts": 1795581000,
  "timeSynced": true,
  "brand": "ExRyze",
  "model": "XR-CORE",
  "firmwareVersion": "1.1.0",
  "sdkVersion": "v5.1.4",
  "chipModel": "ESP32-D0WD- V3",
  "chipRevision": 1,
  "chipCores": 2,
  "cpuFrequency": 240,
  "flashSize": 4194304,
  "sketchSize": 1048576,
  "sketchMd5": "0123456789abcdef0123456789abcdef",
  "loraNodeId": "XR-CORE-01",
  "loraFrequency": 920.0,
  "resetReason": "poweron"
}
```

`resetReason` is a string describing the ESP32 reset cause, such as `poweron`, `panic`, or `wdt`. Device-specific values in the example are illustrative.
