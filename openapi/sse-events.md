# Server-Sent Events (SSE) Reference

Konnected devices running ESPHome firmware expose a real-time event stream using
[Server-Sent Events (SSE)](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events).
This stream delivers live state updates for every entity on the device as they change — no polling
required.

---

## Connecting to the Event Stream

```
GET http://{device-ip-or-hostname}/events
```

The device responds with `Content-Type: text/event-stream` and keeps the connection open
indefinitely. On initial connection, the device immediately sends the current state of **all
entities** as a burst of events, giving you a complete snapshot. Subsequent events are sent only
when a state changes.

### curl

```bash
curl -N http://konnected-abcdef.local/events
```

### JavaScript (EventSource API)

```javascript
const evtSource = new EventSource("http://konnected-abcdef.local/events");

evtSource.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log(data);
};

evtSource.onerror = (err) => {
  console.error("SSE connection error", err);
  // The browser will automatically attempt to reconnect
};
```

### Python

```python
import requests
import json

def listen(host):
    with requests.get(f"http://{host}/events", stream=True, timeout=None) as r:
        for line in r.iter_lines():
            if line.startswith(b"data:"):
                payload = json.loads(line[5:].strip())
                yield payload

for event in listen("konnected-abcdef.local"):
    print(event)
```

---

## Event Payload Structure

Each SSE message is a single line beginning with `data:` followed by a JSON object.

```
data: {"id":"binary-sensor-zone_1","name_id":"binary_sensor/Zone 1","state":"ON","value":true}
```

### Common Fields

| Field | Type | Description |
|---|---|---|
| `id` | string | Entity identifier — **format is changing** (see below) |
| `name_id` | string | New-format identifier — **present only during ESPHome 2026.1.3–2026.7.x** |
| `state` | string | Human-readable state (e.g., `"ON"`, `"OPEN"`, `"-62.0 dBm"`) |
| `value` | varies | Typed value: boolean for binary sensors, number for sensors, string for text sensors |

### Additional Fields by Entity Type

| Entity type | Additional fields |
|---|---|
| Cover (garage door) | `current_operation`: `"IDLE"`, `"OPENING"`, or `"CLOSING"` |
| Light | none beyond common fields |
| Lock | none beyond common fields |
| Select | none beyond common fields |

---

## The Entity ID Transition

The `id` field format is changing across ESPHome versions. Two ESPHome PRs govern this transition:
[#12627](https://github.com/esphome/esphome/pull/12627) and
[#13535](https://github.com/esphome/esphome/pull/13535).

### Version Timeline

| ESPHome version | `id` field | `name_id` field |
|---|---|---|
| ≤ 2026.1.2 | `"binary-sensor-zone_1"` (legacy format) | *(not present)* |
| 2026.1.3 – 2026.7.x | `"binary-sensor-zone_1"` (legacy, unchanged) | `"binary_sensor/Zone 1"` *(new — use this)* |
| ≥ 2026.8.0 | `"binary_sensor/Zone 1"` (new format, permanent) | *(removed)* |

### Legacy `id` Format

```
"{domain}-{object_id}"
```

The domain uses **hyphens** as word separators, and the object_id is the sanitized lowercase slug
derived from the entity's display name (spaces → underscores, non-alphanumeric → underscores).

Examples:
- `"binary-sensor-zone_1"` (Zone 1)
- `"cover-garage_door"` (Garage Door)
- `"select-security__protocol"` (Security+ protocol — note double underscore from `+` and space)
- `"alarm-control-panel-konnected_alarm"` (Konnected Alarm)

### New `id` / `name_id` Format

```
"{domain}/{Entity Display Name}"
```

The domain uses **underscores** (matching the ESPHome YAML key), followed by a `/` separator and
the entity's display name verbatim — **not URL-encoded** in the JSON value (spaces appear as
literal spaces).

Examples:
- `"binary_sensor/Zone 1"`
- `"cover/Garage Door"`
- `"select/Security+ protocol"`
- `"alarm_control_panel/Konnected Alarm"`

### How to Write Forward-Compatible Code

During the transition period, prefer `name_id` when present:

```python
entity_id = event.get("name_id") or event["id"]
```

After 2026.8.0, `name_id` is removed and `id` permanently uses the new format, so the same
line of code continues to work without any modification.

---

## Event Examples

### Alarm Panel Pro

**Zone 1 opens (door opens):**
```json
{"id":"binary-sensor-zone_1","name_id":"binary_sensor/Zone 1","state":"ON","value":true}
```

**Alarm 1 output activated:**
```json
{"id":"switch-alarm_1","name_id":"switch/Alarm 1","state":"ON","value":true}
```

**Warning Beep started:**
```json
{"id":"light-warning_beep","name_id":"light/Warning Beep","state":"ON"}
```

**Alarm system state changes to ARMED_AWAY:**
```json
{"id":"alarm-control-panel-konnected_alarm","name_id":"alarm_control_panel/Konnected Alarm","state":"ARMED_AWAY","value":2}
```

**WiFi signal reading:**
```json
{"id":"sensor-wifi_signal_db","name_id":"sensor/WiFi Signal","state":"-62.0 dBm","value":-62.0}
```

---

### GDO blaQ

**Garage door begins opening:**
```json
{"id":"cover-garage_door","name_id":"cover/Garage Door","state":"OPEN","current_operation":"OPENING","value":1}
```

**Garage door fully open (motor stopped):**
```json
{"id":"cover-garage_door","name_id":"cover/Garage Door","state":"OPEN","current_operation":"IDLE","value":1}
```

**Obstruction detected:**
```json
{"id":"binary-sensor-obstruction","name_id":"binary_sensor/Obstruction","state":"ON","value":true}
```

**Motion detected:**
```json
{"id":"binary-sensor-motion","name_id":"binary_sensor/Motion","state":"ON","value":true}
```

**Garage light turned on:**
```json
{"id":"light-garage_light","name_id":"light/Garage Light","state":"ON"}
```

**Protocol synced:**
```json
{"id":"binary-sensor-synced","name_id":"binary_sensor/Synced","state":"ON","value":true}
```

---

### GDO White

**Wired contact input opens (door detected open):**
```json
{"id":"binary-sensor-garage_door_input","name_id":"binary_sensor/Wired Sensor","state":"ON","value":true}
```

**Laser range sensor distance reading:**
```json
{"id":"sensor-range_sensor","name_id":"sensor/Sensor distance","state":"2.40 m","value":2.40}
```

**STR output energized:**
```json
{"id":"switch-str_output","name_id":"switch/STR output","state":"ON","value":true}
```

---

## How Entity IDs Are Generated

Both the legacy and new identifier formats are **deterministically derived from the entity's
display name** as defined in the firmware configuration at compile time.

**Legacy format derivation (object_id):**
1. Take the entity display name
2. Convert to lowercase
3. Replace every non-alphanumeric character with an underscore
4. Prepend `{domain}-` (domain with hyphens replacing underscores)

**New format derivation (display name):**
The entity display name is used verbatim, prefixed with `{domain}/`.

This means:
- **Renaming an entity requires updating the firmware on the device.**
- After an OTA update or reflash, the entity's URL path and SSE id may change.
- Clients that hardcode paths will break silently.

For integrations that need to survive entity renames or support devices with custom firmware, the
[Endpoint Discovery Pattern](./endpoint-discovery-pattern.md) provides a resilient alternative.

---

## Converting an SSE ID to a REST URL

The new-format entity identifier maps directly to a REST API path. Given a `name_id` (or a
post-2026.8.0 `id`) value:

```
name_id: "binary_sensor/Zone 1"
         ↓
Split on first "/": domain = "binary_sensor", name = "Zone 1"
         ↓
REST path: "/" + domain + "/" + url_encode(name)
         = "/binary_sensor/Zone%201"
```

**Python:**
```python
from urllib.parse import quote

def sse_id_to_rest_path(name_id: str) -> str:
    domain, _, name = name_id.partition("/")
    return f"/{domain}/{quote(name)}"

sse_id_to_rest_path("binary_sensor/Zone 1")     # → "/binary_sensor/Zone%201"
sse_id_to_rest_path("select/Security+ protocol") # → "/select/Security%2B%20protocol"
```

**JavaScript:**
```javascript
function sseIdToRestPath(nameId) {
  const [domain, ...rest] = nameId.split("/");
  return `/${domain}/${encodeURIComponent(rest.join("/"))}`;
}

sseIdToRestPath("binary_sensor/Zone 1");      // → "/binary_sensor/Zone%201"
sseIdToRestPath("cover/Garage Door");         // → "/cover/Garage%20Door"
```

For a complete example of building a dynamically-discovered entity map from the SSE stream, see
the [Endpoint Discovery Pattern](./endpoint-discovery-pattern.md).

---

## Connection Lifecycle

### Initial state burst

When a client connects, the device immediately sends the current state of every entity before
entering streaming mode. This allows clients to initialize their state without a polling round trip.

### Reconnection

If the SSE connection is interrupted (network disruption, device reboot), the client should
reconnect. The browser's `EventSource` API handles this automatically. Python clients should
reconnect in a loop with exponential backoff.

```python
import time, requests, json

def listen_with_reconnect(host, on_event, retry_delay=5):
    while True:
        try:
            with requests.get(f"http://{host}/events", stream=True, timeout=None) as r:
                for line in r.iter_lines():
                    if line.startswith(b"data:"):
                        on_event(json.loads(line[5:].strip()))
        except Exception as e:
            print(f"SSE disconnected: {e}. Reconnecting in {retry_delay}s...")
            time.sleep(retry_delay)
```

### Keepalive

ESPHome sends periodic keepalive pings on the SSE stream to prevent idle connection timeouts.
These appear as lines containing only a colon (`:`) or as empty `data:` events and should be
silently ignored.

---

## Related Resources

- [Migration Guide](./migration-guide.md) — full REST URL migration tables
- [Endpoint Discovery Pattern](./endpoint-discovery-pattern.md) — resilient entity discovery
- [ESPHome Web API Documentation](https://esphome.io/web-api/)
- [ESPHome PR #13535](https://github.com/esphome/esphome/pull/13535) — SSE id transition
