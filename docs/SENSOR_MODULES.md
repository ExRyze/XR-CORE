# Sensor Module Architecture

## Purpose

This document defines the structural contract for sensor modules added to XR-CORE. It is based on the supplied sensor architecture reference and adapted to the current Arduino IDE sketch layout. Sensor modules acquire and validate measurements; they do not own display, web, or transport behavior.

## Source layout

```text
XR-CORE/
├── XR-CORE.ino
├── Sensor_Interface.h
├── Sensor_Types.h
├── Sensor_Manager.h
├── Sensor_Manager.cpp
├── Telemetry_Package.h
├── Telemetry_Manager.h
├── Telemetry_Manager.cpp
├── BME680_Sensor.h
├── BME680_Sensor.cpp
├── PZEM004T_Sensor.h
├── PZEM004T_Sensor.cpp
├── BH1750_Sensor.h
└── BH1750_Sensor.cpp
```

Keep sensor files in the sketch root so Arduino IDE discovers and compiles them with the sketch. Give each concrete sensor a matching header/source pair named after its class. `XR-CORE.ino` remains responsible for object construction, registration, and task orchestration. The filenames above are examples, not a list of sensors selected for this project.

## Module contract

Every sensor implements `SensorInterface` from `Sensor_Interface.h`:

- `begin()` initializes its hardware and reports success.
- `update(now)` performs non-blocking acquisition work and returns `true` only when a new valid reading was committed.
- `isInitialized()`, `isOnline()`, and `isValid()` report distinct lifecycle and data states.
- `lastUpdate()` returns the monotonic time of the last successful measurement.
- `getSnapshot()` returns a coherent copy of status and measurements.
- `printData()` provides diagnostics for development.

Hardware-specific getters and setters belong on the concrete sensor class. Setters should validate configuration and should not expose mutable references to internal measurement state. The common interface intentionally only standardizes lifecycle and snapshot access.

## Data and ownership

Concrete sensors produce a `SensorSnapshot` containing a stable sensor ID, state, timestamp, and named `SensorReading` values with units and validity. The concrete class may also expose a strongly typed data structure and typed getter for sensor-layer code; output subsystems should consume the validated `TelemetryPackage` instead of reading sensor modules directly.

`SensorManager` in `Sensor_Manager.h/.cpp` stores non-owning references: registered sensor objects must outlive it. Registration happens during startup, before the sensor task begins. The manager calls each module's `begin()` and `update(now)`, then combines copied snapshots into a transport-neutral `TelemetrySnapshot`.

Each sensor protects its own mutable state when concurrent access is possible. It copies all fields under a short lock in `getSnapshot()` and does bus or UART operations outside that lock. The manager itself is expected to be updated and read under the application's chosen task synchronization policy; it does not introduce a lock around hardware operations.

## Measurement filtering

Apply stateful filters inside the concrete sensor module, after raw acquisition and basic validity checks, and before committing the filtered values to its snapshot. Keep one filter instance per measurement channel, such as a separate `FilterAVG` for temperature and humidity; never share a stateful filter between sensors or channels. `Sensor_Filter.h/.cpp` provides reusable average, median, and cumulative-mean utilities. `SensorManager` should coordinate lifecycle and aggregation, not select or apply signal-processing algorithms. Whether and how to filter is sensor- and measurement-specific; preserve raw values when consumers need them for diagnostics or calibration.

## Data flow

```text
Sensor modules -> SensorManager -> TelemetryManager -> TelemetryPackage
                                                   ├─ local web
                                                   ├─ OLED display
                                                   ├─ MQTT
                                                   ├─ LoRa
                                                   └─ Arduino IoT Cloud
```

`TelemetryManager` validates the source snapshot and packages its status, validity, timestamp, filter metadata, raw reading, and filtered reading. It records new valid packages in a bounded in-memory history ring. Consumers request the package or history and use its validity flag instead of repeating sensor-specific checks or parsing snapshot readings. They serialize or render the package for their own needs. Sensor classes must not depend on those modules or publish directly. Sensor sampling cadence remains independent from telemetry publication cadence.

## Integration sequence

1. Add the concrete sensor's matching `.h` and `.cpp` files in the sketch root, plus its typed measurement structure.
2. Implement `SensorInterface`, including validated snapshot access.
3. Construct the sensor in `XR-CORE.ino` and register it with `SensorManager` before tasks start.
4. Call manager `begin()` during startup and `update(millis())` from the sensor task.
5. Move consumers to the shared aggregate when integrating physical sensors; define JSON and display mappings at those consumer boundaries.
6. Add only the sensor-specific dependencies and configuration that its hardware requires, keeping source files in the sketch root.

The existing `TelemetryManager` provides the validated BME680 package for the OLED, web, MQTT, and LoRa compatibility value. Additional sensor types can extend the shared snapshot contract and add their own package or mapping without coupling sensor acquisition to presentation or transport code.

## BME680 temperature module

`BME680_Sensor.h/.cpp` is the first concrete module. It reads temperature over the shared I2C bus, starts a forced-mode conversion asynchronously, validates the raw sample, and then filters it. `BME680TemperatureData` stores `rawTemperatureC` and `filteredTemperatureC` separately; both are available from one coherent `getData()` snapshot and as separate readings in `getSnapshot()`. `getTemperature()` remains an alias for the filtered result, with explicit `getRawTemperature()` and `getFilteredTemperature()` getters also available. A Serial debug statement is present but commented out. `TelemetryManager::getTemperaturePackage()` then validates those sensor readings and returns a `TelemetryPackage` with source, measurement, status, validity, timestamp, filter, unit, raw value, and filtered value.

The filter can be selected with `setTemperatureFilter()` or the constructor using `BME680TemperatureFilter::MOVING_AVERAGE`, `MEDIAN`, or `CUMULATIVE_MEAN`. For example, call `bme680Sensor.setTemperatureFilter(BME680TemperatureFilter::MEDIAN);` before `sensorManager.begin()`. Configure it before starting the sensor task; changing it resets filter history and marks the current sample invalid until a new reading is processed. The moving-average and median windows default to five samples and can be set through the constructor's `filterWindowSize` argument (clamped to the filter utility's supported capacity). Cumulative mean ignores the window size. The acquisition interval defaults to two seconds and can be changed with `setReadInterval()` before concurrent use. Failed readings clear the current validity/online status but preserve the last successful raw and filtered values and timestamp for diagnostics. Web, MQTT, and OLED consume `TelemetryPackage`; Arduino IoT Cloud is not connected to this sensor data.
