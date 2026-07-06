# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains ESPHome firmware configurations for Konnected IoT devices (alarm panels and garage door openers). It includes custom ESPHome components written in C++ and Python, along with YAML configuration files.

## Build Commands

Compile a firmware configuration:
```bash
esphome compile <config-file>.yaml
```

Compile and upload to a connected device:
```bash
esphome run <config-file>.yaml
```

Validate a configuration without compiling:
```bash
esphome config <config-file>.yaml
```

## Architecture

### Top-Level YAML Files
Main device firmware templates (e.g., `alarm-panel-pro-esp32-wifi.yaml`, `garage-door-GDOv2-Q.yaml`). These are the entry points imported by ESPHome Dashboard via `dashboard_import`. Each template:
- Defines substitution variables for customization
- Imports packages from `packages/` directory via remote GitHub reference
- Configures device-specific GPIO pin mappings

### Packages (`packages/`)
Reusable YAML modules that provide specific features:
- `core-esp32.yaml`, `core-esp32-s3.yaml`, `core-esp8266.yaml` - Base ESPHome configuration per platform
- `alarm-panel/zone*.yaml` - Individual zone sensor configurations
- `secplus-gdo.yaml` - Complete Security+ garage door opener integration
- Feature packages: `wifi.yaml`, `ethernet.yaml`, `mdns.yaml`, `status-led.yaml`, `warning-beep.yaml`, etc.

### Custom Components (`components/`)

**secplus_gdo** - Security+ garage door opener component (GPLv3):
- `__init__.py` - ESPHome component registration and code generation
- `secplus_gdo.h/.cpp` - Main component class with event callbacks
- Subfolders for ESPHome entity platforms: `cover/`, `light/`, `lock/`, `sensor/`, `binary_sensor/`, `switch/`, `select/`, `number/`
- Depends on external library: `github://konnected-io/gdolib`

**mdns** - Modified mDNS component for Konnected service advertisement

### Component Pattern
Each component subfolder follows ESPHome's component structure:
- `__init__.py` - Python code for YAML schema validation and C++ code generation
- `.h/.cpp` files - C++ implementation for ESP32/ESP8266

### External Dependencies
The secplus_gdo component requires:
```yaml
esphome:
  platformio_options:
    lib_deps:
      - https://github.com/konnected-io/gdolib#v1.0.0
```

## Key Substitution Variables

Common variables used across configs:
- `name` - Device hostname (lowercase, hyphens only)
- `friendly_name` - Human-readable device name
- `project_name` / `project_version` - Identifies firmware variant
- `sensor_debounce_time` - Default sensor debounce (typically 200ms)
- GPIO pin mappings vary by device model

## Local Development

To use local component changes instead of remote GitHub packages:
```yaml
external_components:
  - source:
      type: local
      path: components
    components: [ secplus_gdo ]
```

Comment out the corresponding remote `external_components` reference when testing local changes.

## Device Web UI (`web/gdo-blaq/`)

Custom frontend for the ESPHome internal web server (`web_server: version: 2`), replacing the
stock `https://oi.esphome.io/v2/www.js`. Plain JS/CSS, no framework, no build step:

- `www.js` — the entire app (renders into its own root; hides the stock `<esp-app>` element)
- `www.css` — design tokens + components; dark theme default, light via `data-theme`
- `mockup.html` — dev harness with a simulated device (see below)
- `logo-icon.svg` — brand asset, inlined verbatim into `www.js` (keep in sync if it changes)

Production: the device's HTML shell loads the JS/CSS from the CDN URLs set by the `web_ui_base`
substitution in `garage-door-GDOv2-Q.yaml`. Deploying = uploading `www.js`/`www.css` to that
CDN path (bump the version segment, e.g. `/v1/` → `/v2/`, to bust caches).

### Run / develop locally

```bash
cd web/gdo-blaq && python3 -m http.server 8765
# open http://127.0.0.1:8765/mockup.html  (file:// also works)
```

`mockup.html` stubs the device entirely — fake `EventSource` replaying a real GDOv2-Q entity
snapshot, fake `fetch` handling the REST action endpoints with a simulated door/light/lock —
then loads the local `www.js`/`www.css` relatively. Edit either file and reload. A "SIM" panel
(bottom-right) triggers motion/obstruction pulses, error logs, and connection drops. Set
`EMIT_INTERMEDIATE = false` in `mockup.html` for hardware-faithful cover events (the real
firmware only broadcasts on operation change, not intermediate positions).

To test against a live device, load the page at `http://<device-ip>/` and inject the local
build from the browser console (the stock UI stays hidden via CSS):

```js
const l = Object.assign(document.createElement("link"),
  { rel: "stylesheet", href: "http://127.0.0.1:8765/www.css" });
l.setAttribute("data-gdo-css", "");
document.head.appendChild(l);
document.body.appendChild(Object.assign(document.createElement("script"),
  { src: "http://127.0.0.1:8765/www.js" }));
```

### ESPHome web server API facts the UI depends on

- **Action URLs are built from the SSE `name_id` field** (`"cover/Garage Door"` →
  `POST /cover/Garage%20Door/open`) with legacy object_id fallback — required for
  ESPHome ≥ 2026.7 which removes slug URLs (see `openapi/migration-guide.md`).
- **Incremental `state` broadcasts are slim payloads** — no `domain`/`name`/`entity_category`
  (only the connect replay has them). Always merge into stored entities, never replace.
- **The connect replay can drop events** (small deferred queue, worse with multiple SSE
  clients) and the stream advertises `retry: 30000`. `www.js` runs short second-connection
  "backfill sweeps" after connect to adopt missed entities.
- Cover state is only broadcast on operation change (IDLE↔OPENING/CLOSING), so position
  jumps rather than streaming during travel.
