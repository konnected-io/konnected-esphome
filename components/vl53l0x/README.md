# Konnected VL53L0X component

This component is vendored from ESPHome 2026.10.0 and adds a runtime
measurement watchdog for Konnected GDO White devices.

The stock driver can remain permanently stuck in `waiting_for_interrupt_`
after a transient I2C failure. The watchdog uses the configured measurement
timing budget plus a safety margin, aborts a stale single-shot measurement,
clears the local state machine, publishes `NaN`, and lets the next scheduled
update retry without rebooting the device.

Remove this override after an equivalent fix is released by ESPHome and the
minimum supported ESPHome version includes it.
