# Local web development

This directory contains the npm-based web UI source. The production build is written to the firmware LittleFS `data` directory.

## Commands

```sh
npm install
npm run dev
npm run build
```

Run `npm run build` before uploading the LittleFS image. Keep `data/logo.raw` in place; it is used by the OLED firmware. The build preserves existing files in `data` and only writes the generated HTML, assets, and public logo.

## Dashboard widgets

Dashboard widgets live in `src/widgets/`. A widget module exports a mount function; `src/main.js` finds each matching mount point and passes its `data-*` attributes as configuration. The same widget module can render multiple instances with different API URLs, titles, units, polling intervals, decimal precision, and JSON value paths.

The generic value widget is configured in `index.html` like this:

```html
<div data-widget="value"
     data-api-url="/api/telemetry/{sensor}"
     data-title="Variable"
     data-unit="°C"
     data-poll-interval-ms="2000"
     data-decimals="2"
     data-color="primary"
     data-icon="thermometer"
     data-value-path="value"></div>
```

Add another mount point with different attributes to create another widget instance. Supported colors are `primary`, `success`, `warning`, `danger`, `info`, and `neutral`. Icons are Lucide names registered in `src/main.js`; currently registered options include `activity`, `battery`, `droplets`, `gauge`, `thermometer`, `wind`, and the other icons used by the dashboard. The widget renders only its title and value. The BME680 package endpoint returns:

```json
{
  "source":"bme680",
  "measurement":"temperature",
  "value":24.31,
  "rawValue":24.40,
  "filteredValue":24.31,
  "unit":"C",
  "filter":"MOVING_AVERAGE",
  "initialized":true,
  "online":true,
  "valid":true,
  "timestampMs":123456,
  "updatedAt":123456
}
```

The endpoint requires the same authenticated session as the other device APIs.

### Control and status widgets

The second dashboard row uses the `switch`, `stepper`, `dropdown`, and `status` modules from `src/widgets/`. Each mount point uses the same card shell, title, icon, and color options as the value widget.

```html
<div data-widget="switch" data-title="Grow light" data-icon="lightbulb"
     data-color="warning" data-value="true"
     data-on-label="On · Auto" data-off-label="Off"></div>

<div data-widget="stepper" data-title="Target temperature" data-icon="sliders-horizontal"
     data-color="primary" data-value="22" data-min="16" data-max="30"
     data-step="1" data-unit="°C" data-decimals="0"></div>

<div data-widget="dropdown" data-title="Fan speed" data-icon="fan" data-color="info"
     data-choices='["Auto","Low","Medium","High"]' data-value="Auto"></div>

<div data-widget="status" data-title="Water pump" data-icon="waves"
     data-color="success" data-value="Online" data-detail="Idle · 24 min"></div>
```

Add `data-api-url` to switch, stepper, or dropdown to send changes as a JSON `POST` body containing `{ "value": ... }`. Add `data-api-url` to a status widget to poll a JSON endpoint; configure `data-value-path`, `data-detail-path`, and `data-poll-interval-ms` to select its fields and interval. Without an API URL, the control widgets operate locally and the status widget displays its configured initial value.

### Percentage widget

`src/widgets/percentage.js` uses the shared card template and supports `semicircle`, `three-quarter`, `full`, and `line` variants. Configure each instance with `data-variant`, `data-value`, `data-min`, `data-max`, `data-unit`, `data-color`, and `data-icon`. An optional `data-api-url` enables polling; `data-value-path` selects the value in the JSON response, and `data-detail` supplies supporting text.

```html
<div data-widget="percentage"
     data-api-url="/api/sensor/moisture"
     data-title="Soil moisture"
     data-label="Three quarter"
     data-value-path="value"
     data-value="62"
     data-min="0"
     data-max="100"
     data-unit="%"
     data-variant="three-quarter"
     data-color="success"
     data-icon="sprout"></div>
```

### Chart and image widgets

`src/widgets/chart.js` and `src/widgets/image.js` also use the shared card template. The chart widget accepts `data-ranges` as JSON and can poll a JSON API when `data-api-url` is set. Its API response defaults to `{ "labels": [], "datasets": [] }`; use `data-labels-path` and `data-datasets-path` for nested responses. The selected range is sent as the `range` query parameter by default and can be renamed with `data-range-param`.

```html
<div data-widget="chart"
     data-title="Temperature history"
     data-label="History"
     data-icon="activity"
     data-color="primary"
     data-chart-type="line"
     data-ranges='[{"value":"day","label":"Last 24 hours"},{"value":"week","label":"Last 7 days"}]'
     data-api-url="/api/history"
     data-range-param="range"
     data-poll-interval-ms="10000"></div>
```

The image widget accepts `data-image-url`, `data-alt`, `data-caption`, `data-detail`, and `data-badge`. The image URL can point to a still image or a camera stream supported by the browser.

```html
<div data-widget="image"
     data-title="Greenhouse camera"
     data-icon="image"
     data-color="neutral"
     data-image-url="/camera/stream"
     data-alt="Greenhouse camera view"
     data-caption="Zone A"
     data-detail="Camera 01"
     data-badge="Live"></div>
```
