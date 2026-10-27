/*
 * Copyright (C) 2024  Konnected Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "secplus_gdo.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "inttypes.h"

namespace esphome {
namespace secplus_gdo {

    static const char* const TAG = "secplus_gdo";

    // Runs on gdo_main_task: no publish_state, scheduler, preferences or logging here. Anything
    // ESPHome owns is only safe to touch from the main loop, so the event is queued for loop().
    static void gdo_event_handler(const gdo_status_t* status, gdo_cb_event_t event, void *arg) {
        GDOComponent *gdo = static_cast<GDOComponent *>(arg);

        // The sync retry only talks to gdolib, so it stays on this task to keep its ordering;
        // the result is logged on the main loop
        esp_err_t retry_err = ESP_OK;
        if (event == GDO_CB_EVENT_SYNCED && !status->synced) {
            retry_err = gdo_set_rolling_code(status->rolling_code + 100);
            if (retry_err == ESP_OK) {
                gdo_sync();
            }
        }

        gdo->queue_event(status, event, retry_err);
    }

    void GDOComponent::queue_event(const gdo_status_t *status, gdo_cb_event_t event, esp_err_t retry_err) {
        GDOEvent *ev = this->event_pool_.allocate();
        if (ev == nullptr) {
            // The main loop is stalled; it will republish the current status once it catches up
            this->event_queue_.increment_dropped_count();
            this->refresh_needed_.store(true, std::memory_order_relaxed);
        } else {
            ev->event = event;
            ev->status = *status;
            ev->retry_err = retry_err;
            // Cannot fail: the pool is one smaller than the queue
            this->event_queue_.push(ev);
        }
        // Also wakes the main loop, so the event is handled right away
        this->enable_loop_soon_any_context();
    }

    void GDOComponent::loop() {
        GDOEvent *ev;
        while ((ev = this->event_queue_.pop()) != nullptr) {
            this->handle_event_(*ev);
            this->event_pool_.release(ev);
        }

        uint16_t dropped = this->event_queue_.get_and_reset_dropped_count();
        if (dropped > 0) {
            ESP_LOGW(TAG, "Dropped %u GDO events, republishing current status", dropped);
        }
        if (this->refresh_needed_.exchange(false, std::memory_order_relaxed)) {
            this->republish_status_();
        }

        // An event queued after the last pop has already requested the loop again, and that
        // request is applied after this disable
        this->disable_loop();
    }

    // Runs on the main loop: safe to publish, schedule, save preferences and log
    void GDOComponent::handle_event_(const GDOEvent &ev) {
        const gdo_status_t *status = &ev.status;
        switch (ev.event) {
        case GDO_CB_EVENT_SYNCED:
            ESP_LOGD(TAG, "Synced: %s, protocol: %s", status->synced ? "true" : "false", gdo_protocol_type_to_string(status->protocol));
            if (status->protocol == GDO_PROTOCOL_SEC_PLUS_V2) {
                ESP_LOGD(TAG, "Client ID: 0x%08" PRIX32 " (%" PRIu32 "), Rolling code: %" PRIu32,
                        status->client_id, status->client_id, status->rolling_code);
                if (status->synced) {
                    // Save the last successful ClientID rolling code value to NVS for use on reboot
                    this->set_client_id(status->client_id);
                    this->set_rolling_code(status->rolling_code);
                }
            }

            if (!status->synced) {
                if (ev.retry_err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set rolling code");
                } else {
                    ESP_LOGD(TAG, "Rolling code set to %" PRIu32 ", retrying sync", status->rolling_code + 100);
                }
            } else {
                this->set_protocol_state(status->protocol);
            }

            this->set_sync_state(status->synced);
            break;
        case GDO_CB_EVENT_LIGHT:
            this->set_light_state(status->light);
            break;
        case GDO_CB_EVENT_LOCK:
            this->set_lock_state(status->lock);
            break;
        case GDO_CB_EVENT_DOOR_POSITION: {
            float position = (float)(10000 - status->door_position)/10000.0f;
            this->set_door_state(status->door, position);
            if (status->door != GDO_DOOR_STATE_OPENING && status->door != GDO_DOOR_STATE_CLOSING) {
                this->set_motor_state(GDO_MOTOR_STATE_OFF);
            }
            break;
        }
        case GDO_CB_EVENT_LEARN:
            //ESP_LOGD(TAG, "Learn: %s", gdo_learn_state_to_string(status->learn));
            break;
        case GDO_CB_EVENT_OBSTRUCTION:
            ESP_LOGD(TAG, "Obstruction: %s", gdo_obstruction_state_to_string(status->obstruction));
            this->set_obstruction(status->obstruction);
            break;
        case GDO_CB_EVENT_MOTION:
            ESP_LOGD(TAG, "Motion: %s", gdo_motion_state_to_string(status->motion));
            this->set_motion_state(status->motion);
            break;
        case GDO_CB_EVENT_BATTERY:
            ESP_LOGD(TAG, "Battery: %s", gdo_battery_state_to_string(status->battery));
            break;
        case GDO_CB_EVENT_BUTTON:
            ESP_LOGD(TAG, "Button: %s", gdo_button_state_to_string(status->button));
            this->set_button_state(status->button);
            break;
        case GDO_CB_EVENT_MOTOR:
            ESP_LOGD(TAG, "Motor: %s", gdo_motor_state_to_string(status->motor));
            this->set_motor_state(status->motor);
            break;
        case GDO_CB_EVENT_OPENINGS:
            ESP_LOGD(TAG, "Openings: %d", status->openings);
            this->set_openings(status->openings);
            break;
        case GDO_CB_EVENT_TTC:
            ESP_LOGD(TAG, "Time to close: %d", status->ttc_seconds);
            break;
        case GDO_CB_EVENT_PAIRED_DEVICES:
            ESP_LOGD(TAG, "Paired devices: %d remotes, %d keypads, %d wall controls, %d accessories, %d total",
                    status->paired_devices.total_remotes, status->paired_devices.total_keypads,
                    status->paired_devices.total_wall_controls, status->paired_devices.total_accessories,
                    status->paired_devices.total_all);
            break;
        case GDO_CB_EVENT_OPEN_DURATION_MEASUREMENT:
            ESP_LOGD(TAG, "Open duration: %d", status->open_ms);
            this->set_open_duration(status->open_ms);
            break;
        case GDO_CB_EVENT_CLOSE_DURATION_MEASUREMENT:
            ESP_LOGD(TAG, "Close duration: %d", status->close_ms);
            this->set_close_duration(status->close_ms);
            break;
        default:
            ESP_LOGD(TAG, "Unknown event: %d", ev.event);
            break;
        }
    }

    // Catch up after dropped events. Only the latched states: button presses and duration
    // measurements are one-off edges, and replaying them would re-run their side effects.
    void GDOComponent::republish_status_() {
        gdo_status_t status;
        if (gdo_get_status(&status) != ESP_OK) {
            return;
        }

        if (status.synced) {
            this->set_protocol_state(status.protocol);
            if (status.protocol == GDO_PROTOCOL_SEC_PLUS_V2) {
                this->set_client_id(status.client_id);
                this->set_rolling_code(status.rolling_code);
                this->set_openings(status.openings);
                this->set_motion_state(status.motion);
                this->set_motor_state(status.motor);
            }
        }
        this->set_sync_state(status.synced);
        this->set_door_state(status.door, (float)(10000 - status.door_position)/10000.0f);
        this->set_light_state(status.light);
        this->set_lock_state(status.lock);
        this->set_obstruction(status.obstruction);
    }

    void GDOComponent::setup() {
        // Allocate every event slot now so gdo_main_task never mallocs
        if (!this->event_pool_.warm()) {
            ESP_LOGE(TAG, "Failed to allocate the GDO event pool");
            this->mark_failed();
            return;
        }

        // Set the toggle only state and control here because we cannot guarantee the cover instance was created before the switch
        this->door_->set_toggle_only(this->toggle_only_switch_->state);
        this->toggle_only_switch_->set_control_function(std::bind(&esphome::secplus_gdo::GDODoor::set_toggle_only,
                                                        this->door_, std::placeholders::_1));

        gdo_config_t gdo_conf = {
            .uart_num = UART_NUM_1,
            .obst_from_status = true,
            .invert_uart = true,
            .uart_tx_pin = (gpio_num_t)GDO_UART_TX_PIN,
            .uart_rx_pin = (gpio_num_t)GDO_UART_RX_PIN,
            .obst_in_pin = (gpio_num_t)-1,
        };

        gdo_init(&gdo_conf);
        gdo_get_status(&this->status_);
        if (this->start_gdo_) {
            gdo_start(gdo_event_handler, this);
            ESP_LOGD(TAG, "secplus GDO started!");
        } else {
            // check every 500ms for readiness before starting GDO
            this->set_interval("gdo_start", 500, [this]() {
                if (this->start_gdo_) {
                    gdo_start(gdo_event_handler, this);
                    ESP_LOGD(TAG, "secplus GDO started!");
                    this->cancel_interval("gdo_start");
                }
            });

        }
    }

    void GDOComponent::dump_config() {
        ESP_LOGCONFIG(TAG, "Setting up secplus GDO ...");
    }

    void GDOComponent::set_sync_state(bool synced) {
        if (this->door_) {
            this->door_->set_sync_state(synced);
        }

        if (this->light_) {
            this->light_->set_sync_state(synced);
        }

        if (this->lock_) {
            this->lock_->set_sync_state(synced);
        }

        if (this->f_sync) {
            this->f_sync(synced);
        }
    }

} // namespace secplus_gdo
} // namespace esphome

// Wrap the panic handler to disable the GDO TX pin to prevent spuriously
// triggering the GDO to open when the ESP32 panics. ESPHome 2026.3+ also
// defines __wrap_esp_panic_handler (for crash‑data capture), so our
// __init__.py renames theirs via a preprocessor macro. We #undef it here
// so we can define the real __wrap_esp_panic_handler ourselves and chain
// to ESPHome's (renamed) version after disabling the TX pin.
#undef __wrap_esp_panic_handler
extern "C" {
#include "hal/gpio_ll.h"

void __esphome_wrap_esp_panic_handler(void*);

void __wrap_esp_panic_handler(void* info) {
    esp_rom_printf("PANIC: DISABLING GDO UART TX PIN!\n");
    gpio_ll_func_sel(nullptr, GPIO_NUM_1, PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t)GDO_UART_TX_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en((gpio_num_t)GDO_UART_TX_PIN);

    // Chain to ESPHome's crash handler (captures backtrace, then calls
    // the original IDF panic handler)
    __esphome_wrap_esp_panic_handler(info);
}
} //extern "C"
