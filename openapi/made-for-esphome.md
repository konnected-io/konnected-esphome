# Made for ESPHome

Konnected's second-generation devices run **[ESPHome](https://esphome.io)** — an open-source firmware framework for ESP32 microcontrollers. All firmware configurations are open-source and published on [GitHub](https://github.com/konnected-io/konnected-esphome).

---

## What Is ESPHome?

ESPHome is a firmware configuration system that lets you describe an ESP32-based device in YAML, then compiles and flashes a fully custom binary. It handles the low-level hardware interaction (GPIO, I2C, UART), network connectivity (Wi-Fi, Ethernet), and API exposure automatically, based on the components you declare.

A Konnected device's firmware YAML declares all of its sensors, outputs, and controls as **entities**. Each entity has:
- A **domain** — the type of entity (`binary_sensor`, `switch`, `cover`, `light`, `sensor`, `lock`, `select`, `number`, `button`, `text_sensor`, `alarm_control_panel`)
- A **display name** — the human-readable label shown in dashboards and used in API paths (e.g., `Zone 1`, `Garage Door`, `Alarm 1`)

---

## Two API Options

ESPHome exposes two distinct API interfaces:

### Native API (binary protocol)

ESPHome's native API is a binary protocol designed for tightly integrated clients. It is:
- **Fast and efficient** — binary encoding, persistent TCP connection, push-based updates
- **Feature-rich** — supports all entity types, voice assistant integration, Bluetooth proxy, etc.
- **Complex to implement** — requires a client library (available for Python and others)

Home Assistant's official ESPHome integration uses the native API. If your platform already has a native API client, it may be preferable.

### Web (REST) API — *this documentation*

ESPHome's web server component exposes a simple HTTP/JSON API:
- **Universal** — works from any HTTP client: curl, Python, JavaScript, Node-RED, etc.
- **Self-documenting** — entity paths are derived directly from entity names
- **Stateless polling + SSE push** — poll individual endpoints or subscribe to the event stream

This API Reference is based entirely on ESPHome's Web (REST) API.

---

## Entity Names and API Paths

The REST API path for any entity is derived from its **display name** as defined in the firmware.

### Current URL format (ESPHome ≥ 2026.1.3)

```
/{domain}/{Entity Display Name}
```

The display name is used verbatim, percent-encoded for URL safety:

| Entity name | Domain | URL path |
|---|---|---|
| `Zone 1` | `binary_sensor` | `/binary_sensor/Zone%201` |
| `Garage Door` | `cover` | `/cover/Garage%20Door` |
| `Alarm 1` | `switch` | `/switch/Alarm%201` |
| `Security+ protocol` | `select` | `/select/Security%2B%20protocol` |
| `WiFi Signal %` | `sensor` | `/sensor/WiFi%20Signal%20%25` |

### Legacy URL format (ESPHome < 2026.1.3)

Before 2026.1.3, entity paths used a sanitized "object ID" derived from the display name: lowercase, with non-alphanumeric characters replaced by underscores:

| Entity name | Legacy URL path |
|---|---|
| `Zone 1` | `/binary_sensor/zone_1` |
| `Garage Door` | `/cover/garage_door` |
| `Security+ protocol` | `/select/security__protocol` |

### Compatibility

| ESPHome version | Old URL format | New URL format |
|---|---|---|
| ≤ 2026.1.2 | ✅ Works | ❌ Not available |
| 2026.1.3 – 2026.6.x | ✅ Works (deprecated) | ✅ Works |
| ≥ 2026.7.0 | ❌ Removed (404) | ✅ Works |

**Both formats work on devices currently shipping with ESPHome 2026.4.** The old format will stop working when devices update to ESPHome 2026.7. See the [Migration Guide](./migration-guide) for complete URL change tables and code examples.

---

## SSE Entity IDs

The real-time event stream ([Server-Sent Events](./server-sent-events)) uses a similar identifier:

| ESPHome version | `id` field | `name_id` field |
|---|---|---|
| ≤ 2026.1.2 | `"binary-sensor-zone_1"` (legacy) | *(not present)* |
| 2026.1.3 – 2026.7.x | `"binary-sensor-zone_1"` (legacy) | `"binary_sensor/Zone 1"` *(prefer this)* |
| ≥ 2026.8.0 | `"binary_sensor/Zone 1"` (new, permanent) | *(removed)* |

To write forward-compatible SSE code today:

```python
entity_id = event.get("name_id") or event["id"]
```

This works on all firmware versions: it reads `name_id` when available (2026.1.3+), and falls back to `id` on older firmware. After 2026.8.0, `name_id` disappears and `id` is already in the new format, so the same line continues to work without changes.

---

## Custom Firmware Caveat

Because Konnected's firmware is open-source and based on ESPHome, end users can customize their device configuration. If an entity is **renamed** in a custom firmware build, its URL path and SSE identifier change accordingly. There is no stable opaque entity ID — the display name is the identifier.

For integrations that need to handle custom firmware or survive renames gracefully, the [Endpoint Discovery Pattern](./endpoint-discovery-pattern) provides a resilient approach: connect to the SSE stream, discover entity paths at runtime, and use those paths for subsequent REST calls.

---

## Useful Links

- [ESPHome Documentation](https://esphome.io)
- [ESPHome Web Server Component](https://esphome.io/components/web_server.html)
- [Konnected ESPHome Firmware on GitHub](https://github.com/konnected-io/konnected-esphome)
