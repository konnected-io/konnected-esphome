# Endpoint Discovery Pattern

This guide describes a robust pattern for discovering Konnected device API endpoints at runtime
rather than hardcoding URL paths. This approach is recommended for integrations that need to
handle firmware updates, custom configurations, and device renames without breaking.

---

## The Problem with Hardcoded Paths

Konnected devices expose REST endpoints whose paths are derived from entity display names defined
in the ESPHome firmware. This means:

1. **Firmware updates can change paths.** The ESPHome 2026.7 release changed all URL paths from
   object_id-based slugs to display-name-based paths (see [Migration Guide](./migration-guide.md)).

2. **User renames break paths.** If a device owner renames an entity in their custom firmware
   (e.g., renaming "Zone 1" to "Front Door"), the REST path changes from `/binary_sensor/Zone%201`
   to `/binary_sensor/Front%20Door`. An integration that hardcoded the old path will silently fail.

3. **Custom firmware differs.** Power users may build their own firmware with non-standard entity
   names. No central registry of "canonical" entity IDs exists — the device itself is the source
   of truth.

Hardcoded paths require maintenance whenever entity names change. They also require version
branching to handle both old and new URL formats. The discovery pattern eliminates both problems.

---

## How ESPHome Entity IDs Are Generated

Every entity exposed by an ESPHome device has a display name (e.g., `Zone 1`, `Garage Door`,
`Security+ protocol`). This name, defined in the firmware YAML configuration at compile time,
is the canonical identifier for the entity.

**The ESPHome REST URL is always derived from the display name:**

- **ESPHome ≥ 2026.7:** URL path = `/{domain}/{URL-encoded display name}`
  - `Zone 1` → `/binary_sensor/Zone%201`
- **ESPHome < 2026.7:** URL path = `/{domain}/{object_id}` where object_id = lowercase(name),
  non-alphanumeric → `_`
  - `Zone 1` → `/binary_sensor/zone_1`

**The SSE event `id` is also derived from the display name:**

- **ESPHome ≥ 2026.8.0:** `id` = `"{domain}/{display name}"` (e.g., `"binary_sensor/Zone 1"`)
- **ESPHome 2026.1.3 – 2026.7.x:** Both `id` (legacy) and `name_id` (new) are present
- **ESPHome < 2026.1.3:** `id` = `"{domain with hyphens}-{object_id}"` (e.g., `"binary-sensor-zone_1"`)

The key insight: the **SSE stream tells you the entity's identifier**, and from that identifier you
can derive the correct REST URL — regardless of firmware version or entity name.

---

## The Discovery Pattern

### Overview

1. Connect to the device's SSE stream at `GET /events`
2. Read the initial burst of state events (the device sends current state for all entities on connect)
3. From each event, extract the entity identifier and map it to a REST URL
4. Use the discovered REST URLs for subsequent API calls
5. Reconnect and re-discover if REST calls start returning 404 (indicates entity rename or firmware update)

### Entity Map Structure

Your integration maintains a dictionary mapping a **semantic key** (what the entity means to your
application) to a **REST path** (how to reach it on the device).

```
{
  "garage_door":  "/cover/Garage%20Door",
  "zone_1":       "/binary_sensor/Zone%201",
  "alarm_output": "/switch/Alarm%201",
  ...
}
```

You populate this map during discovery by recognizing entities from their `name_id` (or `id`)
values. The semantic keys and the discovery logic are specific to your application.

---

## Extracting the Identifier

During the transition period (ESPHome 2026.1.3–2026.7.x), SSE payloads contain both `id` and
`name_id`. After 2026.8.0, only `id` is present in the new format.

**Always prefer `name_id` when available, fall back to `id`:**

```python
entity_id = event.get("name_id") or event["id"]
```

This single line is forward-compatible: on old firmware it reads the legacy `id`, on transition
firmware it reads the new `name_id`, and on future firmware (post-2026.8.0) `name_id` is gone
and `id` is already in the new format.

---

## Converting an Entity ID to a REST URL

### From the new format (`name_id` or post-2026.8.0 `id`)

The new-format identifier is `"{domain}/{Entity Name}"`. Split on the first `/` to get the
domain and entity name, then URL-encode the entity name:

```python
from urllib.parse import quote

def entity_id_to_rest_path(entity_id: str) -> str:
    """Convert a new-format entity id to a REST API path."""
    domain, _, name = entity_id.partition("/")
    return f"/{domain}/{quote(name)}"

entity_id_to_rest_path("binary_sensor/Zone 1")      # → "/binary_sensor/Zone%201"
entity_id_to_rest_path("cover/Garage Door")          # → "/cover/Garage%20Door"
entity_id_to_rest_path("select/Security+ protocol")  # → "/select/Security%2B%20protocol"
```

### From the legacy format (pre-2026.1.3 `id`)

The legacy format is `"{hyphen-domain}-{object_id}"`. This is harder to convert to a display
name (you'd need the original name to reverse the sanitization). The recommended approach is:

- Use the legacy `id` directly to construct old-format URLs for devices that don't have `name_id`
- The old URL format is `/{domain}/{object_id}` where domain converts hyphens back to underscores

```python
def legacy_id_to_rest_path(legacy_id: str) -> str:
    """Convert a legacy-format SSE id to an old-format REST path."""
    # Domain mapping: "binary-sensor" → "binary_sensor", "text-sensor" → "text_sensor", etc.
    DOMAIN_MAP = {
        "binary-sensor": "binary_sensor",
        "text-sensor": "text_sensor",
        "alarm-control-panel": "alarm_control_panel",
    }
    parts = legacy_id.split("-")
    # Try two-word domains first
    two_word = f"{parts[0]}-{parts[1]}" if len(parts) >= 2 else ""
    if two_word in DOMAIN_MAP:
        domain = DOMAIN_MAP[two_word]
        object_id = "-".join(parts[2:])
    else:
        domain = parts[0]
        object_id = "-".join(parts[1:])
    return f"/{domain}/{object_id}"
```

---

## Complete Python Example

```python
import json
import time
import threading
from urllib.parse import quote
import requests


class KonnectedDevice:
    """
    A Konnected device client that discovers entity paths via the SSE stream
    and uses them for REST API calls.
    """

    def __init__(self, host: str):
        self.host = host
        self.base_url = f"http://{host}"
        self._entity_map: dict[str, str] = {}  # entity_id → REST path
        self._lock = threading.Lock()
        self._ready = threading.Event()

    def _get_entity_id(self, event: dict) -> str:
        """Extract the best available entity identifier from an SSE event."""
        return event.get("name_id") or event["id"]

    def _id_to_rest_path(self, entity_id: str) -> str:
        """Convert a new-format entity id to a REST path."""
        domain, _, name = entity_id.partition("/")
        if name:
            # New format: "binary_sensor/Zone 1"
            return f"/{domain}/{quote(name)}"
        else:
            # Legacy format: "binary-sensor-zone_1" — convert directly
            return self._legacy_id_to_rest_path(entity_id)

    def _legacy_id_to_rest_path(self, legacy_id: str) -> str:
        """Convert a legacy SSE id to an old-format REST path."""
        MULTI_WORD_DOMAINS = {
            "binary-sensor": "binary_sensor",
            "text-sensor": "text_sensor",
            "alarm-control-panel": "alarm_control_panel",
        }
        for prefix, domain in MULTI_WORD_DOMAINS.items():
            if legacy_id.startswith(prefix + "-"):
                object_id = legacy_id[len(prefix) + 1:]
                return f"/{domain}/{object_id}"
        # Single-word domain: "cover-garage_door", "sensor-wifi_signal_db", etc.
        parts = legacy_id.split("-", 1)
        return f"/{parts[0]}/{parts[1]}" if len(parts) == 2 else f"/{legacy_id}"

    def _connect_sse(self):
        """Connect to the SSE stream and populate the entity map."""
        initial_burst_done = False
        initial_count = 0

        with requests.get(f"{self.base_url}/events", stream=True, timeout=None) as r:
            for raw_line in r.iter_lines():
                if not raw_line or not raw_line.startswith(b"data:"):
                    # Empty line signals end of an SSE event group
                    if not initial_burst_done and initial_count > 0:
                        # A blank line after receiving events signals the burst is complete
                        initial_burst_done = True
                        self._ready.set()
                    continue

                try:
                    event = json.loads(raw_line[5:].strip())
                except (json.JSONDecodeError, IndexError):
                    continue

                entity_id = self._get_entity_id(event)
                rest_path = self._id_to_rest_path(entity_id)

                with self._lock:
                    self._entity_map[entity_id] = rest_path

                initial_count += 1

                # Notify listeners (hook in your own state update logic here)
                self._on_state_update(entity_id, event)

    def _on_state_update(self, entity_id: str, event: dict):
        """Override this method to handle real-time state updates."""
        pass

    def start(self, wait_for_ready: bool = True):
        """Start the SSE listener in a background thread."""
        def _loop():
            while True:
                try:
                    self._connect_sse()
                except Exception as e:
                    print(f"SSE error: {e}. Reconnecting in 5s...")
                    time.sleep(5)

        t = threading.Thread(target=_loop, daemon=True)
        t.start()

        if wait_for_ready:
            self._ready.wait(timeout=15)

    def get_path(self, entity_id: str) -> str | None:
        """Return the discovered REST path for an entity id."""
        with self._lock:
            return self._entity_map.get(entity_id)

    def get(self, entity_id: str) -> dict | None:
        """Fetch current state of an entity by its id."""
        path = self.get_path(entity_id)
        if not path:
            return None
        r = requests.get(f"{self.base_url}{path}", timeout=5)
        return r.json() if r.status_code == 200 else None

    def post(self, entity_id: str, action: str, params: dict = None) -> bool:
        """POST an action to an entity."""
        path = self.get_path(entity_id)
        if not path:
            return False
        url = f"{self.base_url}{path}/{action}"
        r = requests.post(url, params=params, timeout=5)
        return r.status_code == 200


# ── Usage ────────────────────────────────────────────────────────────────────

device = KonnectedDevice("konnected.local")
device.start(wait_for_ready=True)

# Print all discovered entities
for entity_id, path in device._entity_map.items():
    print(f"{entity_id:50s} → {path}")

# Read Zone 1 state (works with both old and new firmware)
zone1 = device.get("binary_sensor/Zone 1")
print(f"Zone 1: {zone1}")

# Open the garage door
device.post("cover/Garage Door", "open")

# Arm the alarm away
device.post("alarm_control_panel/Konnected Alarm", "arm_away")
```

---

## Complete JavaScript Example

```javascript
/**
 * KonnectedDevice — discovers entity paths from the SSE stream and
 * provides REST API methods that work across firmware versions.
 */
class KonnectedDevice {
  constructor(host) {
    this.baseUrl = `http://${host}`;
    this.entityMap = new Map(); // entity_id → REST path
    this._resolveReady = null;
    this.ready = new Promise((resolve) => { this._resolveReady = resolve; });
  }

  /** Extract the best available entity id from an SSE event payload. */
  _getEntityId(event) {
    return event.name_id ?? event.id;
  }

  /** Convert a new-format entity id to a REST path. */
  _idToRestPath(entityId) {
    const slashIdx = entityId.indexOf("/");
    if (slashIdx === -1) {
      // Legacy format: parse and reconstruct old-style path
      return this._legacyIdToRestPath(entityId);
    }
    const domain = entityId.slice(0, slashIdx);
    const name   = entityId.slice(slashIdx + 1);
    return `/${domain}/${encodeURIComponent(name)}`;
  }

  /** Convert a legacy SSE id to an old-format REST path. */
  _legacyIdToRestPath(legacyId) {
    const multiWordDomains = {
      "binary-sensor": "binary_sensor",
      "text-sensor":   "text_sensor",
      "alarm-control-panel": "alarm_control_panel",
    };
    for (const [prefix, domain] of Object.entries(multiWordDomains)) {
      if (legacyId.startsWith(prefix + "-")) {
        return `/${domain}/${legacyId.slice(prefix.length + 1)}`;
      }
    }
    const dashIdx = legacyId.indexOf("-");
    if (dashIdx === -1) return `/${legacyId}`;
    return `/${legacyId.slice(0, dashIdx)}/${legacyId.slice(dashIdx + 1)}`;
  }

  /** Connect to the SSE stream and populate the entity map. */
  connect() {
    const evtSource = new EventSource(`${this.baseUrl}/events`);
    let burstCount = 0;
    let burstTimer = null;

    evtSource.onmessage = (e) => {
      let event;
      try { event = JSON.parse(e.data); } catch { return; }

      const entityId = this._getEntityId(event);
      const restPath = this._idToRestPath(entityId);
      this.entityMap.set(entityId, restPath);
      burstCount++;

      // Signal ready after a short pause with no new events (burst complete)
      clearTimeout(burstTimer);
      burstTimer = setTimeout(() => this._resolveReady(this), 200);

      // Notify state update listeners
      this.onStateUpdate?.(entityId, event);
    };

    evtSource.onerror = () => {
      console.warn("SSE connection lost, browser will reconnect automatically");
    };

    return evtSource;
  }

  /** GET the current state of an entity. */
  async get(entityId) {
    await this.ready;
    const path = this.entityMap.get(entityId);
    if (!path) throw new Error(`Unknown entity: ${entityId}`);
    const r = await fetch(`${this.baseUrl}${path}`);
    return r.json();
  }

  /** POST an action to an entity. */
  async post(entityId, action, params = {}) {
    await this.ready;
    const path = this.entityMap.get(entityId);
    if (!path) throw new Error(`Unknown entity: ${entityId}`);
    const query = new URLSearchParams(params).toString();
    const url = `${this.baseUrl}${path}/${action}${query ? "?" + query : ""}`;
    const r = await fetch(url, { method: "POST" });
    return r.ok;
  }
}

// ── Usage ────────────────────────────────────────────────────────────────────

const device = new KonnectedDevice("konnected.local");
device.connect();

// Set up a real-time state listener
device.onStateUpdate = (entityId, event) => {
  console.log(`[${entityId}] ${event.state}`);
};

// Wait for initial discovery, then use the API
device.ready.then(async () => {
  // Log all discovered entities
  for (const [id, path] of device.entityMap) {
    console.log(`${id.padEnd(50)} → ${path}`);
  }

  // Read Zone 1 state
  const zone1 = await device.get("binary_sensor/Zone 1");
  console.log("Zone 1:", zone1.state);

  // Open garage door
  await device.post("cover/Garage Door", "open");

  // Set Security+ protocol
  await device.post("select/Security+ protocol", "set", { option: "auto" });
});
```

---

## Recognizing Entities by Name Pattern

When building an integration that should work across custom firmware configurations, you may not
know the exact entity names in advance. Rather than matching exact names, match by **domain** and
**name pattern**:

```python
def find_entity(entity_map: dict, domain: str, name_contains: str) -> str | None:
    """Find the REST path for an entity whose id matches domain and name pattern."""
    pattern = f"{domain}/{name_contains}".lower()
    for entity_id, path in entity_map.items():
        if entity_id.lower().startswith(f"{domain}/") and name_contains.lower() in entity_id.lower():
            return path
    return None

# Find any cover entity (garage door) regardless of its exact name
garage_path = find_entity(device._entity_map, "cover", "")
# → "/cover/Garage%20Door" for default firmware
# → "/cover/My%20Garage" for custom firmware
```

---

## Caching and Persistence

For production integrations, cache the discovered entity map to avoid waiting for the SSE burst
on every restart:

```python
import json, pathlib

CACHE_FILE = pathlib.Path("/var/lib/myapp/konnected-entity-map.json")

def save_cache(entity_map):
    CACHE_FILE.write_text(json.dumps(dict(entity_map)))

def load_cache() -> dict:
    try:
        return json.loads(CACHE_FILE.read_text())
    except (FileNotFoundError, json.JSONDecodeError):
        return {}

# On startup: load from cache, connect to SSE, update cache when burst completes
entity_map = load_cache()

def on_burst_complete(new_map):
    entity_map.update(new_map)
    save_cache(entity_map)
```

**Cache invalidation:** If a REST call returns 404 for a path in your cache, the entity was
renamed or removed. Reconnect to the SSE stream to rediscover the updated entity map and refresh
your cache.

---

## Summary

| Approach | Version compatibility | Resilience to renames | Complexity |
|---|---|---|---|
| Hardcoded v2 paths | ESPHome < 2026.7 only | None | Low |
| Hardcoded v3 paths | ESPHome ≥ 2026.7 only | None | Low |
| Try v3 then fallback | Both versions | None | Medium |
| **SSE discovery (this guide)** | **All versions** | **Handles renames** | Medium |

---

## Related Resources

- [Migration Guide](./migration-guide.md) — URL change tables and version compatibility
- [SSE Events Reference](./sse-events.md) — SSE stream details and event payload format
- [Alarm Panel v3 API Spec](./v3/openapi-alarm-panel.yaml)
- [GDO blaQ v3 API Spec](./v3/openapi-gdo-blaq-GDOv2-Q.yaml)
- [GDO White v3 API Spec](./v3/openapi-gdo-white-GDOv2-S.yaml)
