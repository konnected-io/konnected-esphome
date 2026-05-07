# GDO blaQ API Introduction

The **GDO blaQ** (model GDOv2-Q) is a smart garage door retrofit device for Chamberlain and LiftMaster garage door openers that use the **Security+** protocol (Security+ 1.0, Security+ 1.0 with Smart Panel, and Security+ 2.0 / myQ). It connects directly to the opener's wired terminals and communicates over the Security+ wireline protocol — the same protocol used by the opener's own wall controls — giving it complete, native control of the opener.

Unlike a simple relay-based controller, the GDO blaQ knows the true state of the garage door at all times through feedback from the opener itself. No additional sensors are needed.

---

## Supported Openers

The GDO blaQ is compatible with **Chamberlain** and **LiftMaster** garage door openers that use the Security+ protocol, generally those manufactured from approximately 2011 thru 2025. This covers the majority of residential Chamberlain and LiftMaster units sold in North America.

Security+ 2.0 openers (yellow learn button) are fully supported.

Security+ 3.0 openers, introduced in late 2025 (white learn button), are NOT supported by the GDO blaQ.

---

## Key Capabilities

### Garage Door Control

The garage door is represented as a **cover** entity — the ESPHome/Home Assistant term for controllable entryways like garage doors, blinds, and gates.

| Action | Method | URL |
|---|---|---|
| Get door state | GET | `/cover/Garage%20Door` |
| Open | POST | `/cover/Garage%20Door/open` |
| Close | POST | `/cover/Garage%20Door/close` |
| Stop | POST | `/cover/Garage%20Door/stop` |
| Toggle open/closed | POST | `/cover/Garage%20Door/toggle` |
| Set to position | POST | `/cover/Garage%20Door/set?position=0.5` |

The cover state reports:
- `state`: `OPEN` or `CLOSED`
- `current_operation`: `IDLE`, `OPENING`, or `CLOSING`
- `value`: `1` (open) or `0` (closed)

**State is determined by the opener itself** via the Security+ protocol — no separate reed switch or sensor is needed.

### Smart Close Safety Logic

The GDO blaQ automatically executes a **pre-close warning** (audible beeps and LED flash) before closing an open door. This happens whenever:

- A `/close` command is received while the door is open
- A `/toggle` command is received while the door is open
- The close command comes from any source (REST API, physical remote, wall button, or schedule)

You do not need to trigger the pre-close warning yourself before calling `/close`. The firmware handles this automatically. The `/button/Pre-close%20Warning/press` endpoint is available if you want to trigger the warning independently of a close command.

### Garage Light

Control the garage opener's built-in light:

```
GET  /light/Garage%20Light       # Current state
POST /light/Garage%20Light/turn_on
POST /light/Garage%20Light/turn_off
POST /light/Garage%20Light/toggle
```

### Remote Control Lock

Lock or unlock the opener's wireless remote controls. When the Remote Control Lock is active, all physical remotes are disabled. The GDO blaQ and wired wall buttons continue to operate normally regardless of lock state.

```
GET  /lock/Lock            # LOCKED or UNLOCKED
POST /lock/Lock/lock       # Disable remotes
POST /lock/Lock/unlock     # Re-enable remotes
```

### Motion Sensor

If the garage opener is equipped with a motion sensor (standard on many LiftMaster/Chamberlain models), the GDO blaQ reads and exposes its state:

```
GET /binary_sensor/Motion
# state: ON (motion detected) or OFF (no motion)
```

### Safety Beam (Obstruction Sensor)

The photo-eye safety beam state is monitored in real time:

```
GET /binary_sensor/Obstruction
# state: ON (blocked) or OFF (clear)
```

When the beam is blocked, the door will not close.

### Additional Sensors

```
GET /binary_sensor/Motor        # ON while motor is running
GET /binary_sensor/Wall%20Button  # ON while wall button is pressed
GET /binary_sensor/Synced       # ON when GDO blaQ is synced with opener
GET /sensor/Garage%20Openings   # Lifetime opening count from opener
```

The **Synced** sensor is important: the GDO blaQ must synchronize its rolling code with the opener before it can control the door. `Synced: OFF` means initial setup is required or the rolling code has drifted.

### Security+ Protocol Selection

The GDO blaQ auto-detects the Security+ protocol version. In rare cases you may need to set it manually:

```
GET  /select/Security%2B%20protocol
POST /select/Security%2B%20protocol/set?option=auto
POST /select/Security%2B%20protocol/set?option=security%2B2.0
```

Valid options: `auto`, `security+1.0`, `security+1.0 with smart panel`, `security+2.0`

---

## Real-Time Events

All state changes — door movement, light changes, motion events, obstruction alerts — are pushed over the SSE stream in real time. Subscribe once and receive updates as they happen, with no polling needed:

```
GET http://konnected.local/events
```

Example events:
```json
{"id":"cover-garage_door","name_id":"cover/Garage Door","state":"OPEN","current_operation":"OPENING","value":1}
{"id":"binary-sensor-obstruction","name_id":"binary_sensor/Obstruction","state":"ON","value":true}
{"id":"binary-sensor-motion","name_id":"binary_sensor/Motion","state":"ON","value":true}
```

See the [SSE Events Reference](./server-sent-events) for complete documentation.

---

## Learn Mode

The GDO blaQ can control the opener's learn mode to pair new wireless remotes:

```
GET  /switch/Learn           # Learn mode state
POST /switch/Learn/turn_on   # Enable (opener enters learn mode)
POST /switch/Learn/turn_off  # Disable
```

---

## Re-Sync

If the GDO blaQ loses synchronization with the opener's rolling code (visible as `Synced: OFF`), trigger a re-sync:

```
POST /button/Re-sync/press
```

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
| Factory Reset | `POST /button/Factory%20Reset/press` |

---

## URL Format Note

The URLs in this documentation use the **new format** available on devices running ESPHome 2026.1.3 and later (the current shipping version). Both old and new URL formats are accepted through ESPHome 2026.6.x. See the [Migration Guide](./migration-guide) for legacy URL equivalents and compatibility details.

---

## API Specification

The complete OpenAPI specification:
- **v3 (ESPHome ≥ 2026.1.3):** [`openapi/v3/openapi-gdo-blaq-GDOv2-Q.yaml`](../openapi/v3/openapi-gdo-blaq-GDOv2-Q.yaml)
- **v2 (legacy):** [`openapi/v2/openapi-gdo-blaq-GDOv2-Q.yaml`](../openapi/v2/openapi-gdo-blaq-GDOv2-Q.yaml)
