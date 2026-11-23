"""Factory / RMA self-test for the GDO blaQ (GDOv2-Q).

Exercises the hardware that differs from unit to unit -- the Security+ bus
interface and the WiFi radio -- and prints a machine-readable report over the
USB serial log for a host tool (cli-flash-tool) to parse. See
garage-door-GDOv2-Q-selftest.yaml for the fixture this expects.
"""

from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import wifi
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["wifi"]

CONF_TX_PIN = "tx_pin"
CONF_RX_PIN = "rx_pin"
CONF_MIN_RSSI = "min_rssi"
CONF_REPORT_INTERVAL = "report_interval"
CONF_CONTACT_TIMEOUT = "contact_timeout"
CONF_MAX_ATTEMPTS = "max_attempts"
CONF_ON_WAITING_FOR_CONTACT = "on_waiting_for_contact"

gdo_selftest_ns = cg.esphome_ns.namespace("gdo_selftest")
GDOSelfTest = gdo_selftest_ns.class_("GDOSelfTest", cg.Component)
WaitingForContactTrigger = gdo_selftest_ns.class_(
    "WaitingForContactTrigger", automation.Trigger.template(cg.uint8)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GDOSelfTest),
        cv.Required(CONF_TX_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_RX_PIN): pins.internal_gpio_input_pin_number,
        # Lenient on-device floor; the host tool can enforce a stricter,
        # site-specific threshold from the reported scan results.
        cv.Optional(CONF_MIN_RSSI, default=-80): cv.int_range(min=-100, max=0),
        cv.Optional(
            CONF_REPORT_INTERVAL, default="3s"
        ): cv.positive_time_period_milliseconds,
        # How long to wait for the test harness to make contact, per attempt.
        cv.Optional(
            CONF_CONTACT_TIMEOUT, default="45s"
        ): cv.positive_time_period_milliseconds,
        # Bus test attempts before failing, so a slipping hand-held harness
        # gets re-applied rather than failing a good unit.
        cv.Optional(CONF_MAX_ATTEMPTS, default=3): cv.int_range(min=1, max=10),
        # Fires when the unit starts waiting for the harness (variable
        # `attempt`, 1 = first time / ready), and every 10s while it waits.
        cv.Optional(CONF_ON_WAITING_FOR_CONTACT): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                    WaitingForContactTrigger
                ),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pins(config[CONF_TX_PIN], config[CONF_RX_PIN]))
    cg.add(var.set_min_rssi(config[CONF_MIN_RSSI]))
    cg.add(var.set_report_interval(config[CONF_REPORT_INTERVAL]))
    cg.add(var.set_contact_timeout(config[CONF_CONTACT_TIMEOUT]))
    cg.add(var.set_max_attempts(config[CONF_MAX_ATTEMPTS]))
    for conf in config.get(CONF_ON_WAITING_FOR_CONTACT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint8, "attempt")], conf)
    wifi.request_wifi_scan_results()
