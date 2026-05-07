# Konnected Device API Reference

Konnected devices expose a **local REST API** and a **real-time event stream** directly on the device — no cloud required. This reference documents the full API for each Konnected product so you can build integrations, automation workflows, and custom tools that communicate with Konnected hardware on your local network.

## Who This Is For

This documentation targets:

- **Integration developers** building smart home platform connectors (Home Assistant custom integrations, SmartThings drivers, OpenHAB bindings, etc.)
- **Automation engineers** connecting Konnected devices to Node-RED, Home Assistant automations, or custom scripts
- **Makers and developers** who want full programmatic control of their alarm system or garage door opener without relying on cloud services or intermediary platforms

If your platform already has an ESPHome integration, you may prefer ESPHome's **native API** — a binary protocol that offers lower latency and tighter integration. This reference covers the **Web REST API**, which is simpler, JSON-based, and accessible from any HTTP client.

---

## Products Covered

| Product | Description |
|---|---|
| **Alarm Panel** (AP6, AP6-Pro) | 6-zone alarm panel with wired zone inputs and 12V alarm outputs |
| **Alarm Panel Pro** (AP12-Pro) | 12-zone alarm panel with extended zones, dual alarm outputs, and an optional ESPHome alarm control panel |
| **GDO blaQ** (GDOv2-Q) | Smart garage door retrofit for Chamberlain/LiftMaster openers using the Security+ protocol |
| **GDO White** (GDOv2-S, GDOv1-S) | Universal smart garage door opener for any system, using a dry-contact relay and optical laser sensor |

---

## Architecture

Every Konnected device runs **ESPHome** firmware with the `web_server` component enabled. This exposes:

- **REST API** — HTTP GET/POST endpoints for reading sensor states, controlling outputs, and configuring the device
- **Server-Sent Events (SSE)** — a persistent HTTP stream at `/events` that delivers real-time state updates for every entity as they change

See [Made for ESPHome](./made-for-esphome) for background on the firmware platform, and the [SSE Events Reference](./server-sent-events) for the real-time event stream.

---

## Connecting to a Device

Devices are accessible on the local network by IP address or mDNS hostname. The default mDNS hostname follows the pattern `{device-name}.local` — for example, `konnected.local` for a device named `konnected`.

```bash
# Check that a device is reachable
curl http://konnected.local/text_sensor/IP%20Address
```

### Device Discovery

Unconfigured devices broadcast a Wi-Fi setup network named `alarm-panel-xxxxxx` or `garage-door-xxxxxx`. Once configured, devices advertise themselves via **mDNS** (Bonjour/Zeroconf) with:

- Service type `_konnected._tcp` — Konnected-specific service for platform discovery
- Service type `_esphomelib._tcp` — standard ESPHome service type

The mDNS TXT records include the device's project name, firmware version, and friendly name, which integration software can use for automatic discovery.

---

## API URL Format

> **Note on URL format:** Konnected devices were updated to a new URL format in ESPHome 2026.1.3.
> **Both the old and new URL formats work on devices running ESPHome 2026.1.3 through 2026.6.x** (the current version as of this writing is 2026.4). Only the new format will be supported starting with ESPHome 2026.7.
>
> This API reference documents the **new format**. See the [Migration Guide](./migration-guide) for a complete before/after URL table.

REST endpoints follow the pattern:

```
GET  http://{device}:{port}/{domain}/{Entity Name}
POST http://{device}:{port}/{domain}/{Entity Name}/{action}
```

The entity name is the human-readable name assigned to the entity in firmware (e.g., `Zone 1`, `Garage Door`, `Alarm 1`). Names with spaces or special characters must be percent-encoded in HTTP requests:

```
GET http://konnected.local/binary_sensor/Zone%201
POST http://konnected.local/cover/Garage%20Door/open
```

---

## Authentication

Authentication is **disabled by default**. All API endpoints on the local network are accessible without credentials. To enable HTTP digest authentication, customize the firmware with ESPHome's `web_server` configuration:

```yaml
web_server:
  port: 80
  auth:
    username: admin
    password: !secret web_password
```

---

## Response Format

All endpoints return JSON. State responses include at minimum an `id` field and a `state` field:

```json
{
  "id": "binary-sensor-zone_1",
  "name_id": "binary_sensor/Zone 1",
  "state": "ON",
  "value": true
}
```

The `id` field is an entity identifier whose format is transitioning between ESPHome versions (see [Made for ESPHome](./made-for-esphome) for details). The `name_id` field is present on devices running ESPHome 2026.1.3 through 2026.7.x.

---

## Where to Go Next

- [Made for ESPHome](./made-for-esphome) — background on the firmware platform and entity model
- [Alarm Panel Introduction](./alarm-panel-introduction) — Alarm Panel and Alarm Panel Pro API overview
- [GDO blaQ Introduction](./gdo-blaq-introduction) — GDO blaQ API overview
- [GDO White Introduction](./gdo-white-introduction) — GDO White API overview
- [Server-Sent Events Reference](./server-sent-events) — real-time event stream
- [Migration Guide](./migration-guide) — URL format change details
- [Endpoint Discovery Pattern](./endpoint-discovery-pattern) — resilient runtime discovery
