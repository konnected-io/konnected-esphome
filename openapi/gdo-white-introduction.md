# GDO White API Introduction

The **GDO White** (models GDOv2-S and GDOv1-S) is a universal smart garage door opener controller for any existing garage door opener system. Unlike the GDO blaQ — which uses the Security+ protocol to communicate natively with Chamberlain/LiftMaster openers — the GDO White works with any opener by simulating a button press through a **dry-contact relay output**, and determines door state through an **optical laser range sensor** and/or a **wired contact sensor**.

---

## Models

| Model | Availability | Firmware | Notes |
|---|---|---|---|
| **GDOv2-S** | December 2023 – present | ESPHome pre-installed | Ships with ESPHome firmware ready to use |
| **GDOv1-S** | May – December 2023 | Requires flashing | First generation; must be flashed with ESPHome firmware before use |

Both models expose the same REST API. GDOv2-S ships with ESPHome pre-installed; GDOv1-S units require a one-time firmware flash using the [web installer](https://install.konnected.io).

---

## Architecture

### How Door Control Works

The GDO White controls the garage door opener by **momentarily shorting the dry-contact relay** terminals — exactly simulating a wall button press. The firmware triggers the relay when an open, close, stop, or toggle command is issued. This approach works with virtually any opener that has an accessible wired button input.

### How Door State Is Detected

The GDO White determines whether the door is open or closed through one or both of:

**1. Optical Laser Range Sensor (VL53L0X) — GDOv2-S**

A time-of-flight distance sensor mounted on the ceiling measures the distance to the closest surface below it. When the garage door is closed, the door panel is close to the sensor (small distance). When the door is open, the sensor sees a longer distance to the floor or a car parked inside.

You calibrate the sensor by setting the known "open" distance — once set, the sensor automatically determines open vs. closed state by comparing live readings to the calibrated value.

**2. Wired Contact Sensor Input**

A wired dry-contact input (typically a magnetic reed switch mounted on the door) reports open/closed directly. When the magnet moves away from the reed switch (door opens), the input goes to ON.

Both can be used simultaneously. The garage door cover entity combines their readings to determine the overall door state.

---

## Key Capabilities

### Garage Door Control

The garage door is represented as a **cover** entity. The GDO White triggers the opener relay to execute commands, and reports state based on the sensor readings.

| Action | Method | URL |
|---|---|---|
| Get door state | GET | `/cover/Garage%20Door` |
| Open | POST | `/cover/Garage%20Door/open` |
| Close | POST | `/cover/Garage%20Door/close` |
| Stop | POST | `/cover/Garage%20Door/stop` |
| Toggle | POST | `/cover/Garage%20Door/toggle` |

The response includes:
- `state`: `OPEN` or `CLOSED`
- `current_operation`: `IDLE`, `OPENING`, or `CLOSING`
- `value`: `1` (open) or `0` (closed)

**State awareness:** The cover entity is state-aware. If you send a `/close` command to a door that's already closed, no relay pulse is triggered. If you send `/close` to an open door, the firmware automatically triggers the pre-close warning before activating the relay.

### Wired Sensor Input

Read the raw state of the wired contact sensor input directly:

```
GET /binary_sensor/Wired%20Sensor
# state: ON (contact open / door open) or OFF (contact closed / door closed)
```

### Laser Range Sensor (GDOv2-S)

Read the current distance measurement from the VL53L0X sensor:

```
GET /sensor/Sensor%20distance
# Returns distance in meters (e.g., {"state": "2.40 m", "value": 2.40})
```

Read or set the calibrated open-door distance:

```
GET  /number/Sensor%20calibration
POST /number/Sensor%20calibration/set?value=2.40
```

The binary open/closed state derived from the laser sensor (compared to calibrated distance):

```
GET /binary_sensor/Garage%20Door%20Range%20Sensor
# state: ON (door open) or OFF (door closed)
```

### Calibrating the Laser Sensor

1. Open the garage door fully.
2. Read the current sensor distance:
   ```bash
   curl http://konnected.local/sensor/Sensor%20distance
   # {"state": "2.40 m", "value": 2.40}
   ```
3. Set that value as the calibrated open distance:
   ```bash
   curl -X POST "http://konnected.local/number/Sensor%20calibration/set?value=2.40"
   ```

The sensor will now correctly report the door as `OPEN` when the measured distance is within the tolerance margin of `2.40 m`.

### STR Output (12V Strobe)

The GDO White includes a dedicated 12V strobe output terminal (STR) that can drive an external strobe light or other accessory:

```
GET  /switch/STR%20output       # ON or OFF
POST /switch/STR%20output/turn_on
POST /switch/STR%20output/turn_off
POST /switch/STR%20output/toggle
```

### Pre-Close Warning

A pre-close warning sequence (audible beeps + LED flash) runs automatically before the opener is triggered to close. It can also be triggered independently:

```
POST /button/Pre-close%20Warning/press   # Full warning sequence
POST /button/Play%20sound/press          # Audio tone only
```

---

## Real-Time Events

Subscribe to the SSE stream for live state updates — door state changes, sensor readings, output changes:

```
GET http://konnected.local/events
```

Example events:

```json
{"id":"binary-sensor-garage_door_input","name_id":"binary_sensor/Wired Sensor","state":"ON","value":true}
{"id":"cover-garage_door","name_id":"cover/Garage Door","state":"OPEN","current_operation":"IDLE","value":1}
{"id":"sensor-range_sensor","name_id":"sensor/Sensor distance","state":"2.40 m","value":2.40}
```

See the [SSE Events Reference](./server-sent-events) for complete documentation.

---

## Device Information Endpoints

| Entity | URL |
|---|---|
| Device ID | `GET /text_sensor/Device%20ID` |
| IP Address | `GET /text_sensor/IP%20Address` |
| ESPHome version | `GET /text_sensor/ESPHome%20Version` |
| Wi-Fi signal (dBm) | `GET /sensor/WiFi%20Signal` |
| Wi-Fi signal (%) | `GET /sensor/WiFi%20Signal%20%25` |
| Uptime | `GET /sensor/Uptime` |
| Restart | `POST /button/Restart/press` |

---

## URL Format Note

The URLs in this documentation use the **new format** available on devices running ESPHome 2026.1.3 and later (the current shipping version). Both old and new URL formats are accepted through ESPHome 2026.6.x. See the [Migration Guide](./migration-guide) for legacy URL equivalents and compatibility details.

---

## API Specification

The complete OpenAPI specification:
- **v3 (ESPHome ≥ 2026.1.3):** [`openapi/v3/openapi-gdo-white-GDOv2-S.yaml`](../openapi/v3/openapi-gdo-white-GDOv2-S.yaml)
- **v2 (legacy):** [`openapi/v2/openapi-gdo-white-GDOv2-S.yaml`](../openapi/v2/openapi-gdo-white-GDOv2-S.yaml)
