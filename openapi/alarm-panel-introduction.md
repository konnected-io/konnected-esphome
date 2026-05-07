# Alarm Panel API Introduction

The Konnected Alarm Panel and Alarm Panel Pro convert traditional wired alarm system components into smart, software-controlled devices. They connect directly to the wired sensors and alarm outputs of any conventional alarm system, exposing them as a local REST API and real-time event stream.

---

## Models

| Model | Zones | Alarm Outputs | Notes |
|---|---|---|---|
| **Alarm Panel** | 6 | 1 (Siren) | Entry-level, standard 6-zone panel |
| **Alarm Panel Pro** | 12 | 2 (Alarm 1, Alarm 2) | Full 12-zone panel with dual outputs and Ethernet capability |

Both models run similar firmware and expose the same REST API patterns. Zone and output names are numbered accordingly (Zones 1–6 on the standard panel, Zones 1–12 on the Pro).

---

## What the API Controls

### Zone Inputs (binary sensors)

Each zone is a **binary sensor** representing a wired input terminal. Connect any dry-contact or normally-closed sensor:

- Door and window contacts (magnetic reed switches)
- Passive infrared (PIR) motion sensors
- Smoke and CO detectors (dry contact output)
- Water/leak sensors
- Glass-break sensors
- Any other two-wire wired sensor with an open/closed output

The zone reports `ON` when the contact is open (door open, motion detected, sensor triggered) and `OFF` when the contact is closed (door closed, all clear).

```
GET /binary_sensor/Zone%201

{
  "id": "binary-sensor-zone_1",
  "name_id": "binary_sensor/Zone 1",
  "state": "ON",
  "value": true
}
```

Zone 1 through Zone 6 are available on all models. Zones 7–12 are available on the Alarm Panel Pro only.

### Alarm Outputs (switches)

The **Alarm Panel** has a single output named **Siren**, which controls the 12V ALRM terminal — typically connected to a siren, strobe, or other alarm device.

The **Alarm Panel Pro** has two outputs:
- **Alarm 1** — 12V ALRM1 output
- **Alarm 2** — 12V ALRM2 output (can alternatively be configured as a 3.3V trigger via toggle switch)

Outputs are controlled via the `switch` domain:

```
POST /switch/Alarm%201/turn_on   # Activate output
POST /switch/Alarm%201/turn_off  # Deactivate output
POST /switch/Alarm%201/toggle    # Toggle output
GET  /switch/Alarm%201           # Read output state
```

### Warning Beep (light)

All panels include an **OUT1** output wired as a **Warning Beep** — an audible or visual indicator implemented as an ESPHome `light` entity so that strobe effects can produce a repeating beep pattern. On the Alarm Panel Pro, this is the OUT1 terminal. On the 6-zone Alarm Panel, it shares the siren/OUT output.

```
POST /light/Warning%20Beep/turn_on?effect=Strobe   # Repeating beep
POST /light/Warning%20Beep/turn_on                  # Steady on
POST /light/Warning%20Beep/turn_off                 # Off
GET  /light/Warning%20Beep                          # State
```

### Alarm Control Panel (optional)

The ESPHome alarm control panel entity is an **optional** software layer that combines the zone sensors and alarm outputs into a unified arm/disarm interface. When configured, it provides a single entity that manages:

- **Arm modes:** Away, Home, Night, Vacation, Custom Bypass
- **Entry/exit delays:** Configurable per zone
- **Trigger logic:** Automatically triggers alarms based on zone states and arm mode
- **State machine:** DISARMED → ARMING → ARMED\_AWAY/HOME/NIGHT → PENDING → TRIGGERED

```
GET  /alarm_control_panel/Konnected%20Alarm         # Current state
POST /alarm_control_panel/Konnected%20Alarm/arm_away
POST /alarm_control_panel/Konnected%20Alarm/arm_home
POST /alarm_control_panel/Konnected%20Alarm/disarm?code=1234
```

The alarm control panel is configured in firmware and is not present in all builds. The default name is **Konnected Alarm** (path: `/alarm_control_panel/Konnected%20Alarm`).

---

## Real-Time Events

All zone state changes, output activations, and alarm system transitions are broadcast over the SSE stream in real time:

```
GET http://konnected.local/events
```

Example event when a zone opens:
```json
{"id":"binary-sensor-zone_1","name_id":"binary_sensor/Zone 1","state":"ON","value":true}
```

The SSE stream delivers a full state snapshot of all entities immediately on connection, then pushes updates as they occur. See the [SSE Events Reference](./server-sent-events) for full documentation.

---

## Typical Integration Patterns

### Polling for alarm state

For simple automations, poll the alarm control panel or individual zone states on a schedule:

```bash
# Check if any zone is open
curl http://konnected.local/binary_sensor/Zone%201
curl http://konnected.local/binary_sensor/Zone%202
# ...

# Check alarm system state
curl http://konnected.local/alarm_control_panel/Konnected%20Alarm
```

### Subscribing to real-time zone events

For event-driven integrations, subscribe to the SSE stream and react to zone changes:

```javascript
const evtSource = new EventSource("http://konnected.local/events");
evtSource.onmessage = (e) => {
  const data = JSON.parse(e.data);
  const entityId = data.name_id ?? data.id;
  
  if (entityId.startsWith("binary_sensor/Zone")) {
    console.log(`${entityId}: ${data.state}`);
  }
};
```

### Triggering an alarm output

Activate the alarm output and play the warning beep via REST:

```bash
# Start the warning beep (repeating)
curl -X POST "http://konnected.local/light/Warning%20Beep/turn_on?effect=Strobe"

# Activate the alarm output
curl -X POST "http://konnected.local/switch/Alarm%201/turn_on"

# Deactivate after 30 seconds (from script)
sleep 30
curl -X POST "http://konnected.local/switch/Alarm%201/turn_off"
curl -X POST "http://konnected.local/light/Warning%20Beep/turn_off"
```

---

## Device Information Endpoints

All panels expose common device information endpoints:

| Entity | v3 URL (ESPHome ≥ 2026.1.3) |
|---|---|
| Device ID (MAC address) | `GET /text_sensor/Device%20ID` |
| IP Address | `GET /text_sensor/IP%20Address` |
| ESPHome firmware version | `GET /text_sensor/ESPHome%20Version` |
| Wi-Fi signal strength (dBm) | `GET /sensor/WiFi%20Signal` |
| Wi-Fi signal strength (%) | `GET /sensor/WiFi%20Signal%20%25` |
| Device uptime | `GET /sensor/Uptime` |
| Restart device | `POST /button/Restart/press` |

---

## API Specification

The complete OpenAPI specification for the Alarm Panel API is available in:
- **v3 (ESPHome ≥ 2026.1.3 new URL format):** [`openapi/v3/openapi-alarm-panel.yaml`](../openapi/v3/openapi-alarm-panel.yaml)
- **v2 (ESPHome legacy URL format):** [`openapi/v2/openapi-alarm-panel.yaml`](../openapi/v2/openapi-alarm-panel.yaml)
