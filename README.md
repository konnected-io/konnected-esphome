# Konnected for ESPHome

[![GitHub Release](https://img.shields.io/github/v/release/konnected-io/konnected-esphome)](https://github.com/konnected-io/konnected-esphome/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESPHome](https://img.shields.io/badge/ESPHome-%E2%89%A52024.12-informational)](https://esphome.io)

This repository contains the official [ESPHome](https://esphome.io) firmware configurations and custom components for [Konnected](https://konnected.io) devices. It includes ready-to-use YAML templates for all current Konnected hardware, a reusable package library, and the `secplus_gdo` custom component for Security+ garage door openers.

Konnected makes IoT hardware that integrates traditional wired alarm systems and garage door openers into your smart home, primarily through [Home Assistant](https://home-assistant.io).

## Supported Devices

| Device | Hardware Version | Chip | Connectivity | Config File |
|---|---|---|---|---|
| [Alarm Panel](https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit) (6-zone) | — | ESP8266 | WiFi | [`alarm-panel-esp8266.yaml`](alarm-panel-esp8266.yaml) |
| [Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) (12-zone) | v1.5–v1.7 | ESP32 | WiFi | [`alarm-panel-pro-esp32-wifi.yaml`](alarm-panel-pro-esp32-wifi.yaml) |
| [Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) (12-zone) | v1.5–v1.7 | ESP32 | Ethernet | [`alarm-panel-pro-esp32-ethernet.yaml`](alarm-panel-pro-esp32-ethernet.yaml) |
| [Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) (12-zone) | v1.8+ | ESP32 | Ethernet | [`alarm-panel-pro-v1.8-ethernet.yaml`](alarm-panel-pro-v1.8-ethernet.yaml) |
| [Garage Door Opener](https://konnected.io/products/smart-garage-door-opener) | GDOv1-S | ESP8266 | WiFi | [`garage-door-GDOv1-S.yaml`](garage-door-GDOv1-S.yaml) |
| [Garage Door Opener](https://konnected.io/products/smart-garage-door-opener) | GDOv2-S | ESP32-S3 | WiFi | [`garage-door-GDOv2-S.yaml`](garage-door-GDOv2-S.yaml) |
| [GDO blaQ](https://konnected.io/products/smart-garage-door-opener-blaq-myq-alternative) | GDOv2-Q | ESP32-S3 | WiFi | [`garage-door-GDOv2-Q.yaml`](garage-door-GDOv2-Q.yaml) |

> **Which Alarm Panel Pro ethernet config?** Check the hardware version printed on the front of the device beneath the logo. Use `alarm-panel-pro-esp32-ethernet.yaml` for v1.5, v1.6, and v1.7. Use `alarm-panel-pro-v1.8-ethernet.yaml` for v1.8 and newer. These are separate config files — no manual variable editing is needed.

> **GDOv2-S vs GDO blaQ?** The GDOv2-S works with any traditional garage opener using a dry-contact relay and an optical range sensor. The GDO blaQ communicates directly with Security+ and Security+ 2.0 openers (Chamberlain, LiftMaster, Craftsman, Merlin since 1997) over a serial connection — no dry-contact wiring required.

## Quick Start

1. **Flash firmware** using the web-based tool at [install.konnected.io/esphome](https://install.konnected.io/esphome) — no software installation required.
2. **Power on** the device.
3. **Connect to WiFi** (WiFi devices): join the captive portal network named `konnected-XXXXXX`, select your home WiFi network, and enter the password. Ethernet devices skip this step — just plug in the cable.
4. **Discover in Home Assistant**: go to [Settings → Devices & Services](https://my.home-assistant.io/redirect/integrations) and accept the discovered Konnected device.
5. **(Optional) Customize in ESPHome Dashboard**: import the device into your [ESPHome Dashboard](https://my.home-assistant.io/redirect/supervisor_store/) to edit the configuration, add packages, or build custom firmware.

> **ESPHome version requirement:** ESPHome **2024.12.0 or newer** is required when building or modifying firmware using ESPHome Dashboard.

## Changes and Release Notes

See [Releases](https://github.com/konnected-io/konnected-esphome/releases) for release notes and pre-built flashable firmware images.

---

## Device Firmware Configurations

### Alarm Panel — `alarm-panel-esp8266.yaml`

![Alarm Panel](https://PLACEHOLDER_ALARM_PANEL_IMAGE_URL)

Konnected's original [6-zone Alarm Panel](https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit) is a compact WiFi-only modular alarm panel. Multiple panels can be daisy-chained for larger systems. This config also runs on Konnected v1 hardware (2018–2020) and NodeMCU development boards.

- **Zones:** 6 input zones (`zone1`–`zone6`) + 1 combined alarm/output (`alarm` / `out`)
- **Default packages:** core-esp8266, WiFi, mDNS, status LED, zones 1–6, alarm output, warning beep

---

### Alarm Panel Pro (WiFi) — `alarm-panel-pro-esp32-wifi.yaml`

![Alarm Panel Pro](https://PLACEHOLDER_ALARM_PANEL_PRO_IMAGE_URL)

Konnected's flagship [12-zone Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) powered by ESP32.

- **Zones:** 12 input zones (`zone1`–`zone12`) + 2 alarm outputs (`alarm1`, `alarm2`) + 2 auxiliary outputs (`out1`, `out2`)
- **Default packages:** core-esp32, WiFi, mDNS, status LED, zones 1–12, alarm outputs 1–2, warning beep

---

### Alarm Panel Pro (Ethernet, v1.5–v1.7) — `alarm-panel-pro-esp32-ethernet.yaml`

Same as above but connected via Ethernet using the **LAN8720** PHY. Use this config for hardware versions **v1.5, v1.6, and v1.7**.

- **Default packages:** core-esp32, Ethernet, mDNS, status LED, zones 1–12, alarm outputs 1–2, warning beep

---

### Alarm Panel Pro (Ethernet, v1.8+) — `alarm-panel-pro-v1.8-ethernet.yaml`

Alarm Panel Pro **v1.8 and newer** uses the **RTL8201** PHY and requires this dedicated config file. It also applies a `phy_registers` workaround for a known hardware issue in the 2406 production batch. This workaround is harmless on newer batches (2408+).

- **Default packages:** same as above, with RTL8201 Ethernet driver and PHY register fix

---

### Alarm Panel Pro — Local Alarm Mode — `examples/alarm-panel-pro-local-alarm.yaml`

A configuration example showing the Alarm Panel Pro operating as a **fully self-contained alarm system** using ESPHome's native [`alarm_control_panel`](https://esphome.io/components/alarm_control_panel/) component. No cloud connection or Home Assistant automation logic is required — arming, entry delay, and siren triggering all run on-device.

**Features:**
- Named zones with appropriate `device_class` (door, motion, sound)
- Arm away / arm home / arm night modes
- Per-zone bypass configuration per arm mode (e.g. motion zones bypassed when armed home)
- Entry delay (pending state) triggers the warning beep
- Alarm output (`alarm1`) fires on trigger
- Disarm with PIN code via Home Assistant or the ESPHome API
- State restored after power loss

```yaml
alarm_control_panel:
  - platform: template
    name: Alarm Panel
    codes:
      - "1234"
    binary_sensors:
      - input: zone1
      - input: zone2
      - input: zone4
        bypass_armed_home: true
        bypass_armed_night: true
    restore_mode: RESTORE_DEFAULT_DISARMED
    on_triggered:
      then:
        - switch.turn_on: alarm1
    on_cleared:
      then:
        - switch.turn_off: alarm1
```

See the full config at [`examples/alarm-panel-pro-local-alarm.yaml`](examples/alarm-panel-pro-local-alarm.yaml). This is the recommended starting point for local-only alarm setups.

> **Note:** The root-level `alarm-panel-pro-esp32-local-alarm.yaml` is an older implementation using a third-party state machine component. It is no longer maintained. Use the `examples/` version above instead.

---

### Garage Door Opener (GDOv1-S) — `garage-door-GDOv1-S.yaml`

![Garage Door Opener](https://PLACEHOLDER_GDOV1S_IMAGE_URL)

The original [Konnected Garage Door Opener](https://konnected.io/products/smart-garage-door-opener) for traditional garage openers. Uses a dry-contact relay to simulate a wall button press and detects door state with a wired contact sensor and/or a VL53L0X optical laser range sensor. Built on ESP8266.

**Sensor mode** — choose one cover package depending on your hardware:

| Mode | Package | When to use |
|---|---|---|
| Range sensor only *(default)* | `garage-door-cover-range.yaml` | Optical range sensor installed; no wired sensor |
| Wired sensor only | `garage-door-cover-wired.yaml` | Wired contact sensor connected to INPUT terminal |
| Both sensors | `garage-door-cover-wired-and-range.yaml` | Both sensors installed |

To switch modes, comment out the default cover package line and uncomment the desired one. The corresponding sensor package (`vl53l0x-range-sensor.yaml` or `garage-door-wired-sensor.yaml`) can likewise be removed if that sensor is not in use.

- **Default packages:** core-esp8266, garage door wired sensor, VL53L0X range sensor, garage door cover (range mode), opener button, switch, warning LED, pre-close warning, WiFi, mDNS, status LED, warning beep

---

### Garage Door Opener (GDOv2-S) — `garage-door-GDOv2-S.yaml`

![Garage Door Opener GDOv2-S](https://PLACEHOLDER_GDOV2S_IMAGE_URL)

The second-generation [Konnected Garage Door Opener](https://konnected.io/products/smart-garage-door-opener) built on the ESP32-S3-MINI. Uses the same dry-contact relay approach as the GDOv1-S with the same three sensor mode options. Supports BLE provisioning via [Improv](https://www.improv-wifi.com/).

**Sensor mode:** same three cover package options as the GDOv1-S — see the table above.

**Pre-close warning:** plays a configurable RTTTL melody tone via the onboard piezo buzzer (compared to the simple repeating beep on the GDOv1-S).

- **Default packages:** core-esp32-s3, garage door wired sensor, VL53L0X range sensor, garage door cover (range mode), opener button, switch, warning LED, buzzer RTTTL + pre-close warning tones, WiFi, mDNS, status LED

---

### GDO blaQ (GDOv2-Q) — `garage-door-GDOv2-Q.yaml`

![GDO blaQ](https://PLACEHOLDER_GDO_BLAQ_IMAGE_URL)

Konnected's [GDO blaQ](https://konnected.io/products/smart-garage-door-opener-blaq-myq-alternative) connects directly to the garage opener unit over the Security+ serial protocol — no dry contacts, no separate sensor wiring. Connect just two wires to the wall button terminals on the opener.

This device uses the [`secplus_gdo`](#secplus_gdo-component) custom component. Compatible with **Security+** and **Security+ 2.0** openers sold since 1997 under the Chamberlain, LiftMaster, Craftsman, and Merlin brands.

**Home Assistant entities:**

| Entity | Type | Description |
|---|---|---|
| Garage Door | Cover | Open / close / stop the door |
| Garage Light | Light | Toggle the garage opener light |
| Lock | Lock | Lock / unlock remote controls |
| Motion | Binary Sensor | Motion detected by the opener unit |
| Obstruction | Binary Sensor | Obstruction sensor state |
| Motor | Binary Sensor | Motor running state |
| Wall Button | Binary Sensor | Physical wall button press events |
| Synced | Binary Sensor | Serial communication sync status |
| Garage Openings | Sensor | Cumulative open/close count |
| Learn | Switch | Put opener into learn mode |
| Toggle Only | Switch | Compatibility mode for openers that only respond to toggle commands (see [Toggle-Only Mode](#toggle-only-mode)) |
| Security+ Protocol | Select | Protocol version — auto-detected, manual override available |
| Pre-close Warning | Button | Trigger the warning LED and buzzer sequence |
| Re-sync | Button | Re-establish communication with the opener |
| Reset Door Timings | Button | Clear learned open/close travel durations |
| Factory Reset | Button | Reset device to factory defaults |

**Pre-close warning:** configurable RTTTL melody + LED strobe plays before any automatic close command, per [16 CFR 1211.14(f)](https://www.ecfr.gov/current/title-16/part-1211#p-1211.14(f)).

**Hardware batch notes:**
- Batch **2403 and older**: uncomment `hardware_uart: UART0` in the `logger:` section of your config.
- Batch **2404 and newer**: no change needed.

- **Default packages:** core-esp32-s3, WiFi, mDNS, warning LED, buzzer RTTTL + pre-close warning tones, status LED, secplus-gdo, debug

---

## Customization

### How Packages Work

Each top-level config file pulls in reusable feature modules called _packages_ from the `packages/` directory via ESPHome's remote package mechanism:

```yaml
packages:
  remote_package:
    url: https://github.com/konnected-io/konnected-esphome
    ref: master
    refresh: 5min
    files:
      - packages/core-esp32.yaml
      - packages/wifi-esp32.yaml
      # ... more packages
```

- **Disable a feature:** comment out its package line.
- **Add a feature:** add a package line.
- **Override a package's behavior:** copy the package's contents inline into your config file and remove the package import line, then edit freely.

---

### Substitution Variables

All top-level config files expose commonly customized settings as substitution variables at the top of the file.

#### Common to all devices

| Variable | Default | Description |
|---|---|---|
| `name` | `konnected` | Device hostname. Lowercase letters, numbers, and hyphens only. A MAC suffix is appended automatically. |
| `friendly_name` | varies | Human-readable name displayed in Home Assistant. |
| `sensor_debounce_time` | `200ms` | [Debounce filter](https://esphome.io/components/binary_sensor/gpio.html#debouncing-values) duration applied to all binary sensor inputs. |
| `blink_on_state` | `"true"` | Whether the status LED blinks on sensor state changes. |

#### Alarm Panel — warning beep

| Variable | Default | Description |
|---|---|---|
| `warning_beep_pulse_time` | `100ms` | Duration of each beep pulse. |
| `warning_beep_pause_time` | `130ms` | Quiet pause between beep pulses. |
| `warning_beep_internal_only` | `"false"` | When `"true"`, the warning beep entity is not exposed to Home Assistant. |
| `warning_beep_shared` | `"false"` | When `"true"`, the beep shares the alarm output pin. |
| `warning_beep_pin` | `$out1` (ESP32) / `$out` (ESP8266) | GPIO pin driving the piezo buzzer. |

#### Garage Door Opener (GDOv1-S / GDOv2-S)

| Variable | Default | Description |
|---|---|---|
| `garage_door_cover_name` | `Garage Door` | Name of the Cover entity in Home Assistant. |
| `switch_name` | `Switch` / `STR output` | Name of the STR switched output entity. |
| `garage_door_opener_momentary_duration` | `300ms` | How long the relay closes to simulate a button press. |
| `garage_door_close_warning_duration` | `5s` | Duration of the pre-close warning sequence. |
| `open_garage_door_distance_margin_of_error` | `0.1` / `0.05` | Tolerance in meters for the range sensor open position detection. |
| `range_sensor_polling_time` | `2500ms` | How often the VL53L0X range sensor takes a reading. |
| `range_sensor_debounce_time` | `3s` | Debounce duration for range sensor state changes. |

#### GDO blaQ (GDOv2-Q)

| Variable | Default | Description |
|---|---|---|
| `garage_door_cover_name` | `Garage Door` | Name of the Cover entity. |
| `garage_light_name` | `Garage Light` | Name of the Light entity. |
| `garage_openings_name` | `Garage Openings` | Name of the openings counter sensor. |
| `garage_lock_name` | `Lock` | Name of the Lock entity. |
| `garage_motion_name` | `Motion` | Name of the motion binary sensor. |
| `garage_obstruction_name` | `Obstruction` | Name of the obstruction binary sensor. |
| `garage_motor_name` | `Motor` | Name of the motor binary sensor. |
| `garage_button_name` | `Wall Button` | Name of the wall button binary sensor. |
| `garage_sync_name` | `Synced` | Name of the sync status binary sensor. |
| `garage_door_close_warning_duration` | `5s` | Duration of the pre-close warning sequence. |

---

### Zone Customization (Alarm Panel)

All zone binary sensors are defined in packages (`packages/alarm-panel/zone1.yaml`, etc.) with generic names like "Zone 1". Use ESPHome's `!extend` to rename zones and set the correct `device_class` without copying or modifying the package:

```yaml
binary_sensor:
  - id: !extend zone1
    name: Front Door
    device_class: door

  - id: !extend zone2
    name: Back Door
    device_class: door

  - id: !extend zone3
    name: Motion - Living Room
    device_class: motion

  - id: !extend zone5
    name: Glassbreak - Kitchen
    device_class: sound
```

This is the recommended way to customize zones. A complete real-world example is in [`examples/alarm-panel-pro-local-alarm.yaml`](examples/alarm-panel-pro-local-alarm.yaml).

---

### Packages Reference

#### Core / Platform

| Package | Description |
|---|---|
| `core-esp32.yaml` | Required base for all ESP32 devices: device identity, OTA updates, uptime sensor, restart button. |
| `core-esp32-s3.yaml` | Same as above, for ESP32-S3. |
| `core-esp8266.yaml` | Same as above, for ESP8266. |

#### Connectivity

| Package | Description |
|---|---|
| `wifi.yaml` | WiFi connectivity for ESP8266 + signal strength diagnostics. |
| `wifi-esp32.yaml` | WiFi connectivity for ESP32 + signal strength diagnostics. |
| `ethernet.yaml` | Ethernet connectivity. Uses the `ethernet_type` substitution. |
| `mdns.yaml` | Advertises a `_konnected._tcp` mDNS/Zeroconf service used by the Konnected app for device discovery. |

#### Alarm Panel

| Package | Description |
|---|---|
| `alarm-panel/zone1.yaml` … `zone12.yaml` | Individual input zone binary sensors. |
| `alarm-panel/alarm.yaml` | Single alarm output switch (ESP8266). |
| `alarm-panel/alarm1.yaml`, `alarm2.yaml` | Dual alarm output switches (ESP32). |

#### UI / Indicators

| Package | Description |
|---|---|
| `status-led.yaml` | Onboard status LED: blinks on activity, solid on error. |
| `warning-beep.yaml` | Warning beep entity implemented as a strobe light effect on a piezo output. |
| `warning-led.yaml` | Warning LED strobe entity. |
| `pre-close-warning.yaml` | Simple repeating beep pre-close warning (GDOv1-S). |
| `buzzer-rtttl.yaml` | RTTTL melody player using the onboard piezo buzzer. |
| `pre-close-warning-tones.yaml` | RTTTL pre-close warning button, used with `buzzer-rtttl.yaml` (GDOv2-S / GDO blaQ). |

#### Garage Door

| Package | Description |
|---|---|
| `garage-door-wired-sensor.yaml` | Wired contact sensor on the INPUT terminal. |
| `vl53l0x-range-sensor.yaml` | VL53L0X optical laser range sensor for door state detection. |
| `garage-door-cover-range.yaml` | Garage door Cover entity using range sensor for open/closed state. |
| `garage-door-cover-wired.yaml` | Garage door Cover entity using wired contact sensor for state. |
| `garage-door-cover-wired-and-range.yaml` | Garage door Cover entity using both sensors for state. |
| `garage-door-opener-button.yaml` | Dry-contact relay as a momentary button entity. |
| `switch.yaml` | STR 12V switched output as a switch entity. |
| `secplus-gdo.yaml` | Full Security+ GDO entity set for the GDO blaQ. |

#### GDO Add-ons (`packages/gdo/`)

These optional packages extend the GDO blaQ with additional hardware integrations.

| Package | Description |
|---|---|
| `gdo/gdo-external-toggle.yaml` | Connect an external dry-contact button or relay to Input 1 or Input 2 to toggle the door — useful for integrating legacy systems with a relay output. |
| `gdo/3v-output.yaml` | Configure a GDO input pin as a 3.3V power supply for an external sensor. |
| `gdo/external-binary-sensor.yaml` | Add a generic binary sensor on a GDO input pin. |
| `gdo/garage-door-binary-sensor.yaml` | Add a wired contact sensor on a GDO input pin for door state. |

#### Diagnostics / Misc

| Package | Description |
|---|---|
| `debug.yaml` | Exposes heap free memory and last reboot reason as diagnostic entities. |
| `konnected-cloud.yaml` | MQTT/TLS connection to Konnected Cloud on AWS IoT. Credentials are provisioned automatically by the Konnected app and are not available outside that managed build process. |
| `mqtt-disable.yaml` | Adds a button entity to toggle the cloud MQTT connection on/off. |

---

## secplus_gdo Component

The `secplus_gdo` ESPHome component implements communication with Security+ and Security+ 2.0 garage door openers over the reverse-engineered serial wireline protocol. It powers the [GDO blaQ](https://konnected.io/products/smart-garage-door-opener-blaq-myq-alternative) and is maintained by Konnected.

### Importing the Component

```yaml
external_components:
  - source: github://konnected-io/konnected-esphome@master
    refresh: 5min
    components: [ secplus_gdo ]
```

Also add the `gdolib` dependency to your `esphome:` block:

```yaml
esphome:
  platformio_options:
    lib_deps:
      - https://github.com/konnected-io/gdolib#v1.0.0
```

### Core Configuration

```yaml
secplus_gdo:
  id: gdo_blaq
  input_gdo_pin: GPIO2   # uart_rx_pin
  output_gdo_pin: GPIO1  # uart_tx_pin
```

### Supported Platforms

See [`components/secplus_gdo/README.md`](components/secplus_gdo/README.md) and [`packages/secplus-gdo.yaml`](packages/secplus-gdo.yaml) for complete working examples.

| Platform | Notes |
|---|---|
| `cover` | Options: `pre_close_warning_duration`, `pre_close_warning_start`, `pre_close_warning_end`, `toggle_only` |
| `light` | Standard ESPHome light entity for the garage opener's built-in light |
| `lock` | Lock / unlock remote controls |
| `binary_sensor` | `type`: `motion`, `obstruction`, `motor`, `button`, `sync` |
| `sensor` | `type`: `openings` |
| `switch` | `type`: `learn`, `toggle_only` |
| `select` | Security+ protocol version (auto-detected) |
| `number` | `type`: `open_duration`, `close_duration`, `client_id`, `rolling_code` |
| `button` | Factory reset, re-sync, reset door timings |

### Toggle-Only Mode

Some openers (notably certain Merlin models) do not respond to discrete open/close commands and require a toggle command instead. Enable the **Toggle Only** switch entity in Home Assistant or configure it in YAML:

```yaml
cover:
  - platform: secplus_gdo
    name: Garage Door
    secplus_gdo_id: gdo_blaq
    toggle_only: true
```

### Attribution

This component was adapted from [ratgdo](https://paulwieland.github.io/ratgdo/) and the [secplus](https://github.com/argilo/secplus) reverse-engineering work. It is a complete rewrite — not a fork — and is published under the GPLv3 license. Key differences from ratgdo: event-driven (not polling) for faster state response; two-wire installation with obstruction state read over serial; automatic Security+ vs Security+ 2.0 detection; pre-closing warning compliant with [16 CFR 1211.14(f)](https://www.ecfr.gov/current/title-16/part-1211#p-1211.14(f)).

---

## Advanced / Example Configurations

The `examples/` directory contains ready-to-use configurations for specific use cases. Import them into ESPHome Dashboard to use.

| Example | Description |
|---|---|
| [`examples/alarm-panel-pro-local-alarm.yaml`](examples/alarm-panel-pro-local-alarm.yaml) | Alarm Panel Pro as a fully local alarm system with named zones, multiple arm modes, entry delay, and siren output. **Recommended starting point for local alarm setups.** |
| [`examples/GDOv2-S-wired-sensor-only.yaml`](examples/GDOv2-S-wired-sensor-only.yaml) | GDOv2-S configured for wired contact sensor only, with the optical range sensor disabled. |
| [`examples/gdo-blaq-offline-dry-contact-toggle-relay.yaml`](examples/gdo-blaq-offline-dry-contact-toggle-relay.yaml) | GDO blaQ operating fully offline — no WiFi, no API — with an external dry-contact relay on Input 1 to toggle the door. Useful for legacy system integration. |
| [`examples/alarm-panel-pro-kiosk-alarm.yaml`](examples/alarm-panel-pro-kiosk-alarm.yaml) | Alarm Panel Pro as a self-contained alarm for a kiosk enclosure, armed and disarmed automatically by an electronically controlled latch rather than a user-entered code. |
| [`examples/esp32-s3-devkit-c-interface-paradox-evo192.yaml`](examples/esp32-s3-devkit-c-interface-paradox-evo192.yaml) | Demonstrates using Konnected packages on a third-party ESP32-S3-DevKitC-1 board to bridge a traditional Paradox EVO 192 alarm panel to Home Assistant. |

---

## REST API

Each device exposes a local HTTP REST API when the built-in web server is enabled (`web_server:` in the config). Konnected publishes OpenAPI specifications for this API:

- **Alarm Panel:** [`openapi-alarm-panel.yaml`](openapi-alarm-panel.yaml)
- **GDO blaQ:** [`openapi-gdo-blaq-GDOv2-Q.yaml`](openapi-gdo-blaq-GDOv2-Q.yaml)
- **Garage Door Opener (GDOv2-S):** [`openapi-gdo-white-GDOv2-S.yaml`](openapi-gdo-white-GDOv2-S.yaml)

Full API documentation is at [konnected.readme.io](https://konnected.readme.io).

---

## Local Development

### Prerequisites

- Python 3.9+
- ESPHome ≥ 2024.12.0: `pip install esphome`

### Build Commands

```bash
# Validate config without compiling
esphome config <config-file>.yaml

# Compile firmware
esphome compile <config-file>.yaml

# Compile and flash to a connected device
esphome run <config-file>.yaml
```

### Using Local Components

When iterating on the `secplus_gdo` component, swap the remote `external_components` source for the local `components/` directory:

```yaml
external_components:
  # Comment out the remote source:
  # - source: github://konnected-io/konnected-esphome@master
  #   components: [ secplus_gdo ]

  # Use the local source instead:
  - source:
      type: local
      path: components
    components: [ secplus_gdo ]
```

### Using Local Packages

When iterating on packages, reference them locally with `!include` instead of the remote package block:

```yaml
packages:
  core: !include packages/core-esp32.yaml
  wifi: !include packages/wifi-esp32.yaml
  # ... etc.
```

### secrets.yaml

Configs that reference secrets (such as the local alarm PIN code) require a `secrets.yaml` file in your working directory:

```yaml
# secrets.yaml
alarm_master_code: "1234"
```

---

## Contributing

Contributions are welcome. The repository structure follows ESPHome conventions:

- **Top-level YAML files** are per-device firmware templates imported via `dashboard_import`.
- **`packages/`** contains reusable feature modules. Keep each package focused on a single feature.
- **`components/secplus_gdo/`** is the custom C++ component. See its [README](components/secplus_gdo/README.md) for component development details.
- **`examples/`** contains real-world customization configurations.

When proposing new packages or component changes, open a pull request with a description of the use case and any hardware requirements.

**Licenses:** YAML configs and packages are MIT licensed. The `secplus_gdo` component is GPLv3 licensed.

---

## Get Help

Konnected customers can contact Konnected Support at [help.konnected.io](https://help.konnected.io) or [help@konnected.io](mailto:help@konnected.io). Support is available to verified purchasers of Konnected hardware.

For general ESPHome questions, see the [ESPHome documentation](https://esphome.io) and [Home Assistant community forums](https://community.home-assistant.io/c/esphome).
