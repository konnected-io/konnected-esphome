# API Migration Guide: ESPHome URL Format Change (2026.7)

ESPHome 2026.1.3 introduced a new URL format for the REST API — and the old format will be removed in ESPHome 2026.7. This guide explains what changed, what you can do today, and what will break in 2026.7 if you don't migrate.

> **Devices running the current firmware (ESPHome 2026.4) already support both URL formats.**
> You can start using the new URLs today, and they will continue to work after 2026.7.
> Only the old-format URLs will break when 2026.7 ships.

## What Changed

ESPHome's local REST API previously identified entities using a sanitized "object ID" derived from
the entity name: all characters are lowercased and non-alphanumeric characters are replaced with
underscores. For example, an entity named **Zone 1** was accessible via:

```
GET http://konnected.local/binary_sensor/zone_1
```

Starting with **ESPHome 2026.1.3**, entity URLs also accept the entity's **display name** directly:

```
GET http://konnected.local/binary_sensor/Zone%201
```

The underlying logic is straightforward: take the display name as-is and percent-encode any
characters that are not valid in a URL path segment. The entity name `Zone 1` becomes `Zone%201`
because the space character encodes as `%20`.

The old object_id-based URLs are deprecated and will be removed in **ESPHome 2026.7**.

**Two separate but related changes ship across versions:**

1. **HTTP/REST URL paths** — new display-name format added in 2026.1.3; old object_id format removed in 2026.7
2. **SSE event `id` field** — new `name_id` field added in 2026.1.3; `id` permanently adopts new format in 2026.8

---

## Timeline

| Milestone | ESPHome version | What happens |
|---|---|---|
| **New URLs available** | 2026.1.3 | New display-name URLs begin working alongside old URLs. SSE gains a new `name_id` field. Old URLs still work but log deprecation warnings. |
| **Current version** | **2026.4** | Both old and new URL formats accepted. Both `id` and `name_id` in SSE payloads. |
| **Breaking: REST** | **2026.7.0** | Old object_id REST URLs are **removed**. Requests to old URLs return 404. |
| **Breaking: SSE** | **2026.8.0** | SSE `name_id` field removed. `id` field permanently adopts the new slash/display-name format. |

**Recommendation:** Migrate to the new URL format now. It works on all current devices (ESPHome 2026.1.3+) and is the only format that will work after 2026.7.

---

## HTTP/REST URL Changes

### URL Format Rules

| Component | Before (ESPHome < 2026.7) | After (ESPHome ≥ 2026.7) |
|---|---|---|
| Domain | Unchanged (`binary_sensor`, `cover`, etc.) | Unchanged |
| Entity identifier | Sanitized slug: lowercase, non-alphanumeric → `_` | URL-encoded display name |
| Action segment | Unchanged (`turn_on`, `open`, `press`, etc.) | Unchanged |

**Encoding rules for display names:**
- Space → `%20`
- `+` → `%2B`
- `%` → `%25`
- Forward slash `/` is now **prohibited** in entity names (ESPHome validation error)

### Alarm Panel / Alarm Panel Pro

| Entity | v2 URL (ESPHome < 2026.7) | v3 URL (ESPHome ≥ 2026.7) |
|---|---|---|
| Zone 1 | `/binary_sensor/zone_1` | `/binary_sensor/Zone%201` |
| Zone 2 | `/binary_sensor/zone_2` | `/binary_sensor/Zone%202` |
| Zone 3 | `/binary_sensor/zone_3` | `/binary_sensor/Zone%203` |
| Zone 4 | `/binary_sensor/zone_4` | `/binary_sensor/Zone%204` |
| Zone 5 | `/binary_sensor/zone_5` | `/binary_sensor/Zone%205` |
| Zone 6 | `/binary_sensor/zone_6` | `/binary_sensor/Zone%206` |
| Zone 7 (Pro) | `/binary_sensor/zone_7` | `/binary_sensor/Zone%207` |
| Zone 8 (Pro) | `/binary_sensor/zone_8` | `/binary_sensor/Zone%208` |
| Zone 9 (Pro) | `/binary_sensor/zone_9` | `/binary_sensor/Zone%209` |
| Zone 10 (Pro) | `/binary_sensor/zone_10` | `/binary_sensor/Zone%2010` |
| Zone 11 (Pro) | `/binary_sensor/zone_11` | `/binary_sensor/Zone%2011` |
| Zone 12 (Pro) | `/binary_sensor/zone_12` | `/binary_sensor/Zone%2012` |
| Alarm 1 output | `/switch/alarm_1` | `/switch/Alarm%201` |
| Alarm 2 output | `/switch/alarm_2` | `/switch/Alarm%202` |
| Alarm 1 turn on | `/switch/alarm_1/turn_on` | `/switch/Alarm%201/turn_on` |
| Alarm 2 turn on | `/switch/alarm_2/turn_on` | `/switch/Alarm%202/turn_on` |
| Warning Beep state | `/light/warning_beep` | `/light/Warning%20Beep` |
| Warning Beep on | `/light/warning_beep/turn_on` | `/light/Warning%20Beep/turn_on` |
| Alarm system state | `/alarm_control_panel/konnected_alarm` | `/alarm_control_panel/Konnected%20Alarm` |
| Arm away | `/alarm_control_panel/konnected_alarm/arm_away` | `/alarm_control_panel/Konnected%20Alarm/arm_away` |
| WiFi Signal dBm | `/sensor/wifi_signal_rssi` | `/sensor/WiFi%20Signal` |
| WiFi Signal % | `/sensor/wifi_signal__` | `/sensor/WiFi%20Signal%20%25` |
| Uptime | `/sensor/uptime` | `/sensor/Uptime` |
| Device ID | `/text_sensor/device_id` | `/text_sensor/Device%20ID` |
| IP Address | `/text_sensor/ip_address` | `/text_sensor/IP%20Address` |
| Restart | `/button/restart/press` | `/button/Restart/press` |

### GDO blaQ (GDOv2-Q)

| Entity | v2 URL (ESPHome < 2026.7) | v3 URL (ESPHome ≥ 2026.7) |
|---|---|---|
| Garage door state | `/cover/garage_door` | `/cover/Garage%20Door` |
| Open | `/cover/garage_door/open` | `/cover/Garage%20Door/open` |
| Close | `/cover/garage_door/close` | `/cover/Garage%20Door/close` |
| Stop | `/cover/garage_door/stop` | `/cover/Garage%20Door/stop` |
| Toggle | `/cover/garage_door/toggle` | `/cover/Garage%20Door/toggle` |
| Set position | `/cover/garage_door/set?position=0.5` | `/cover/Garage%20Door/set?position=0.5` |
| Garage light | `/light/garage_light` | `/light/Garage%20Light` |
| Remote lock state | `/lock/lock` | `/lock/Lock` |
| Lock remotes | `/lock/lock/lock` | `/lock/Lock/lock` |
| Unlock remotes | `/lock/lock/unlock` | `/lock/Lock/unlock` |
| Motion sensor | `/binary_sensor/motion` | `/binary_sensor/Motion` |
| Obstruction sensor | `/binary_sensor/obstruction` | `/binary_sensor/Obstruction` |
| Motor running | `/binary_sensor/motor` | `/binary_sensor/Motor` |
| Wall button | `/binary_sensor/wall_button` | `/binary_sensor/Wall%20Button` |
| Protocol synced | `/binary_sensor/synced` | `/binary_sensor/Synced` |
| Garage openings | `/sensor/garage_openings` | `/sensor/Garage%20Openings` |
| Protocol setting | `/select/security__protocol` | `/select/Security%2B%20protocol` |
| Set protocol | `/select/security__protocol/set` | `/select/Security%2B%20protocol/set` |
| Learn mode | `/switch/learn` | `/switch/Learn` |
| Pre-close warning | `/button/pre_close_warning/press` | `/button/Pre-close%20Warning/press` |
| Play sound | `/button/play_sound/press` | `/button/Play%20sound/press` |
| Restart | `/button/restart/press` | `/button/Restart/press` |
| Factory Reset | `/button/factory_reset/press` | `/button/Factory%20Reset/press` |

### GDO White (GDOv2-S)

| Entity | v2 URL (ESPHome < 2026.7) | v3 URL (ESPHome ≥ 2026.7) |
|---|---|---|
| Garage door state | `/cover/garage_door` | `/cover/Garage%20Door` |
| Open / Close / Stop / Toggle | `/cover/garage_door/{action}` | `/cover/Garage%20Door/{action}` |
| Wired sensor | `/binary_sensor/wired_sensor` | `/binary_sensor/Wired%20Sensor` |
| Range sensor state | `/binary_sensor/garage_door_range_sensor` | `/binary_sensor/Garage%20Door%20Range%20Sensor` |
| STR output state | `/switch/str_output` | `/switch/STR%20output` |
| STR turn on | `/switch/str_output/turn_on` | `/switch/STR%20output/turn_on` |
| Sensor distance | `/sensor/sensor_distance` | `/sensor/Sensor%20distance` |
| Calibration value | `/number/sensor_calibration` | `/number/Sensor%20calibration` |
| Set calibration | `/number/sensor_calibration/set?value=2.4` | `/number/Sensor%20calibration/set?value=2.4` |
| Pre-close warning | `/button/pre_close_warning/press` | `/button/Pre-close%20Warning/press` |
| Play sound | `/button/play_sound/press` | `/button/Play%20sound/press` |
| Restart | `/button/restart/press` | `/button/Restart/press` |

---

## SSE Event ID Changes

In addition to the REST URL changes, the entity identifier in Server-Sent Events payloads is also
changing. See the full [SSE Events Reference](./sse-events.md) for complete documentation. Here is
a summary of the `id` field evolution:

### Before (ESPHome ≤ 2026.1.2)

```
data: {"id":"binary-sensor-zone_1","state":"ON","value":true}
```

The `id` format is `"{domain}-{object_id}"` where the domain uses hyphens (e.g., `binary-sensor`)
and the object_id is the sanitized lowercase slug.

### During transition (ESPHome 2026.1.3 – 2026.7.x)

```
data: {"id":"binary-sensor-zone_1","name_id":"binary_sensor/Zone 1","state":"ON","value":true}
```

**Both** `id` (legacy format) **and** `name_id` (new format) are present simultaneously. The
legacy `id` continues to work for backward compatibility. Clients should begin using `name_id`
for forward compatibility.

### After (ESPHome ≥ 2026.8.0)

```
data: {"id":"binary_sensor/Zone 1","state":"ON","value":true}
```

The `name_id` field is removed. The `id` field permanently adopts the new format:
`"{domain}/{Entity Name}"` where the domain uses underscores and the entity name is the display
name verbatim (not URL-encoded; spaces appear as literal spaces in the JSON value).

### SSE `id` Format Reference

| Domain | Old `id` prefix | New `id` / `name_id` prefix |
|---|---|---|
| `binary_sensor` | `binary-sensor-` | `binary_sensor/` |
| `sensor` | `sensor-` | `sensor/` |
| `text_sensor` | `text-sensor-` | `text_sensor/` |
| `switch` | `switch-` | `switch/` |
| `light` | `light-` | `light/` |
| `cover` | `cover-` | `cover/` |
| `lock` | `lock-` | `lock/` |
| `select` | `select-` | `select/` |
| `number` | `number-` | `number/` |
| `button` | `button-` | `button/` |
| `alarm_control_panel` | `alarm-control-panel-` | `alarm_control_panel/` |

---

## Why Entity Names Are Not Stable

Both REST URLs and SSE ids are derived deterministically from the entity's **display name** as
defined in the firmware at compile time. This means:

- If a user or developer renames an entity in their firmware configuration and reflashes the
  device, **all URL paths and SSE ids for that entity will change**.
- There is no stable opaque identifier. The display name *is* the identifier.
- The default Konnected firmware uses consistent names (documented in the API spec), so for
  stock firmware builds this is predictable. Custom firmware may differ.

**Recommendation:** Instead of hardcoding URL paths, use the SSE-based endpoint discovery pattern
described in [Endpoint Discovery Pattern](./endpoint-discovery-pattern.md). This approach reads
entity identifiers directly from the device at runtime and handles both URL format versions
transparently.

---

## Migration Strategies

### Strategy 1: Direct update (simplest)

Replace all hardcoded old-format URLs with new-format URLs using the tables above. Update
any SSE `id` parsing to handle both `name_id` (if present) and `id`.

This works well if:
- You control the firmware and know entities won't be renamed
- You only need to support devices running a single ESPHome version

**Detecting the version at runtime:**

```
GET http://konnected.local/text_sensor/ESPHome%20Version   (v3 devices)
GET http://konnected.local/text_sensor/esphome_version      (v2 devices)
```

If the v3 endpoint returns 200, the device is running 2026.7+. If it returns 404, fall back to
the v2 endpoint. Parse the version string to determine which URL format to use.

```python
import requests

def get_esphome_version(host):
    # Try v3 URL first
    r = requests.get(f"http://{host}/text_sensor/ESPHome%20Version", timeout=5)
    if r.status_code == 200:
        return r.json()["state"]
    # Fall back to v2 URL
    r = requests.get(f"http://{host}/text_sensor/esphome_version", timeout=5)
    if r.status_code == 200:
        return r.json()["state"]
    return None

version = get_esphome_version("konnected.local")
use_v3_urls = version and tuple(int(x) for x in version.split(".")[:2]) >= (2026, 7)
```

### Strategy 2: Try-new-then-fallback

Attempt the v3 URL first; if it returns 404, retry with the v2 URL. This makes the client
transparently compatible with both device generations without needing to check the version first.

```python
def get_entity(host, domain, v3_name, v2_slug):
    """
    Fetch entity state, trying v3 URL first and falling back to v2.
    """
    from urllib.parse import quote
    v3_url = f"http://{host}/{domain}/{quote(v3_name)}"
    r = requests.get(v3_url, timeout=5)
    if r.status_code == 200:
        return r.json()
    # Fall back to v2 object_id slug
    v2_url = f"http://{host}/{domain}/{v2_slug}"
    return requests.get(v2_url, timeout=5).json()

# Example
state = get_entity("konnected.local", "binary_sensor", "Zone 1", "zone_1")
```

### Strategy 3: Runtime endpoint discovery (recommended for integrations)

Connect to the SSE stream, discover entity identifiers from the initial state burst, and use
those identifiers to construct REST URLs. This approach is fully version-transparent and handles
custom entity names.

See the complete [Endpoint Discovery Pattern](./endpoint-discovery-pattern.md) guide.

---

## curl Examples

### Alarm Panel Pro — Zone 1 state

```bash
# v2 (ESPHome < 2026.7)
curl http://konnected.local/binary_sensor/zone_1

# v3 (ESPHome ≥ 2026.7)
curl "http://konnected.local/binary_sensor/Zone%201"
```

### Alarm Panel Pro — Trigger Alarm 1

```bash
# v2
curl -X POST http://konnected.local/switch/alarm_1/turn_on

# v3
curl -X POST "http://konnected.local/switch/Alarm%201/turn_on"
```

### GDO blaQ — Close garage door

```bash
# v2
curl -X POST http://konnected.local/cover/garage_door/close

# v3
curl -X POST "http://konnected.local/cover/Garage%20Door/close"
```

### GDO blaQ — Set Security+ protocol

```bash
# v2
curl -X POST "http://konnected.local/select/security__protocol/set?option=auto"

# v3
curl -X POST "http://konnected.local/select/Security%2B%20protocol/set?option=auto"
```

### GDO White — Read laser sensor distance

```bash
# v2
curl http://konnected.local/sensor/sensor_distance

# v3
curl "http://konnected.local/sensor/Sensor%20distance"
```

---

## JavaScript Examples

```javascript
const BASE = "http://konnected.local";

// Fetch zone state (v3)
async function getZoneState(zone) {
  const path = encodeURIComponent(`Zone ${zone}`);
  const r = await fetch(`${BASE}/binary_sensor/${path}`);
  return r.json();
}

// Open garage door (v3)
async function openGarageDoor() {
  await fetch(`${BASE}/cover/${encodeURIComponent("Garage Door")}/open`, {
    method: "POST"
  });
}

// Compatible SSE handler (reads name_id when present, falls back to id)
function connectSSE(onEvent) {
  const evtSource = new EventSource(`${BASE}/events`);
  evtSource.onmessage = (e) => {
    const data = JSON.parse(e.data);
    const entityId = data.name_id ?? data.id;
    onEvent(entityId, data.state, data.value);
  };
  return evtSource;
}
```

---

## Related Resources

- [Endpoint Discovery Pattern](./endpoint-discovery-pattern.md)
- [SSE Events Reference](./sse-events.md)
- [Alarm Panel v3 API Spec](./v3/openapi-alarm-panel.yaml)
- [GDO blaQ v3 API Spec](./v3/openapi-gdo-blaq-GDOv2-Q.yaml)
- [GDO White v3 API Spec](./v3/openapi-gdo-white-GDOv2-S.yaml)
- [ESPHome PR #12627 — URL format change](https://github.com/esphome/esphome/pull/12627)
- [ESPHome PR #13535 — SSE id transition](https://github.com/esphome/esphome/pull/13535)
