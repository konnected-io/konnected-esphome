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
substitution in `garage-door-GDOv2-Q.yaml`. Deploy with:

```bash
scripts/deploy-web-ui.sh --dry-run     # show what would change
scripts/deploy-web-ui.sh               # publish (prompts before uploading)
```

Assets live in the `app.konnected.io` S3 bucket behind CloudFront, scoped by platform and
model so other product UIs can share the bucket:

- `https://app.konnected.io/esp/gdo-blaq/www.js` — rolling latest, `max-age=300`, invalidated
  on every deploy. This is what firmware points at, so UI fixes ship without reflashing.
- `https://app.konnected.io/esp/gdo-blaq/<version>/www.js` — immutable snapshot,
  `max-age=31536000`. Point `web_ui_base` here to freeze the UI with a firmware build.

Each deploy writes both. The script refuses to overwrite an existing snapshot whose content
differs (bump `--version`, or `--force`), then verifies the live bytes against local. Do not
pre-gzip — CloudFront compresses on the fly (brotli/gzip).

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

- **Entity identity is `name_id || id`, never `id` alone.** Both spell
  `"{domain}/{Display Name}"`: ESPHome 2026.1.3–2026.7.x send `name_id` alongside a
  legacy `id` (`"{domain}-{object_id}"`), and **2026.8.0 drops `name_id` and makes `id`
  itself the name form** (see `openapi/migration-guide.md`). Reading `id` alone silently
  breaks every object_id lookup on 2026.8 — issue #131: the safety strip and Device card
  render empty while anything found by domain scan still works. `www.js` stores entities
  under `name_id || id` and indexes them by the object_id slug it recomputes from the
  display name (lowercase; anything outside `[a-z0-9_-]` → `_`, matching ESPHome's
  `to_sanitized_char(to_snake_case_char(c))`). Action URLs come from the same pair
  (`"cover/Garage Door"` → `POST /cover/Garage%20Door/open`).
  `mockup.html` has a `WIRE` constant to replay either format — check both after
  touching entity lookup.
- **Incremental `state` broadcasts are slim payloads** — no `domain`/`name`/`entity_category`
  (only the connect replay has them). Always merge into stored entities, never replace.
- **The connect replay can drop events** (small deferred queue, worse with multiple SSE
  clients) and the stream advertises `retry: 30000`. `www.js` runs short second-connection
  "backfill sweeps" after connect to adopt missed entities.
- Cover state is only broadcast on operation change (IDLE↔OPENING/CLOSING), so position
  jumps rather than streaming during travel.
- **A `binary_sensor` has no "unknown" on the wire.** `binary_sensor_json_()` serializes
  `obj->state` unconditionally, so "never published" and "OFF" are identical bytes.
  Numeric `sensor`s *do* carry it (state `"NA"`, value `null` from NaN), and so do `number`s.

### Availability rules the UI applies

Two separate reasons a value may not be showable, handled differently in `www.js`:

- **Unknown** → dim + disable (`.is-unknown`, `[data-unsynced]`). Everything the opener
  owns (door, light, lock, obstruction, motor, wall button, cycles) holds a constructor
  default until the Security+ handshake lands, and the firmware *silently drops* commands
  until then — `GDODoor` / `GDOLight` / `GDOLock` `control()` return early while `!synced_`.
  Because binary sensors can't express unknown (above), the `synced` binary sensor is the
  only trustworthy signal. It is set once by `gdo_sync_task` and republished on each failed
  re-sync attempt; the Settings card deliberately stays live, since the protocol select and
  Re-sync are how you recover.
- **Unsupported** → hide outright. Security+ 1.0 wall-panel frames carry only door, light,
  lock, obstruction and wall button (gdolib `decode_v1_packet`). Motion, motor, openings
  and learn mode are Security+ 2.0-only and hard-gated in gdolib (`update_motion_state`
  bails on non-v2; `gdo_activate_learn` returns `ESP_ERR_NOT_SUPPORTED`). Detected from
  the protocol `select` value (`security+1.0`, `security+1.0 with smart panel`).
- **Motion sensor presence** is optional hardware with no discovery mechanism — an opener
  without one just never sends a motion frame, which is indistinguishable from "clear".
  The UI shows the tile only after motion has been reported once, and remembers that per
  device in `localStorage` under `konnected-gdo-motion:<device_id>`.
