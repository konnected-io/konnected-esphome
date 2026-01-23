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

**mqtt** - Modified MQTT component

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
