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

    static void gdo_event_handler(const gdo_status_t* status, gdo_cb_event_t event, void *arg) {
        GDOComponent *gdo = static_cast<GDOComponent *>(arg);
        switch (event) {
        case GDO_CB_EVENT_SYNCED:
            ESP_LOGI(TAG, "Synced: %s, protocol: %s", status->synced ? "true" : "false", gdo_protocol_type_to_string(status->protocol));
            if (status->protocol == GDO_PROTOCOL_SEC_PLUS_V2) {
                ESP_LOGI(TAG, "Client ID: %" PRIu32 ", Rolling code: %" PRIu32, status->client_id, status->rolling_code);
                gdo->set_client_id(status->client_id);
                gdo->set_rolling_code(status->rolling_code);
            }

            if (!status->synced) {
                if (gdo_set_rolling_code(status->rolling_code + 100) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set rolling code");
                } else {
                    ESP_LOGI(TAG, "Rolling code set to %" PRIu32 ", retryng sync", status->rolling_code);
                    gdo_sync();
                }
            } else {
                gdo->set_protocol_state(status->protocol);
            }

            break;
        case GDO_CB_EVENT_LIGHT:
            ESP_LOGI(TAG, "Light: %s", gdo_light_state_to_string(status->light));
            gdo->set_light_state(status->light);
            break;
        case GDO_CB_EVENT_LOCK:
            ESP_LOGI(TAG, "Lock: %s", gdo_lock_state_to_string(status->lock));
            gdo->set_lock_state(status->lock);
            break;
        case GDO_CB_EVENT_DOOR_POSITION: {
            float position = (float)(10000 - status->door_position)/10000.0f;
            float target = (float)(10000 - status->door_target)/10000.0f;
            ESP_LOGI(TAG, "Door: %s, %.0f%%, target: %.0f%%", gdo_door_state_to_string(status->door),
                     position*100, target*100);
            gdo->set_door_state(status->door, position);
            if (status->door != GDO_DOOR_STATE_OPENING && status->door != GDO_DOOR_STATE_CLOSING) {
                gdo->set_motor_state(GDO_MOTOR_STATE_OFF);
            }
            break;
        }
        case GDO_CB_EVENT_LEARN:
            ESP_LOGI(TAG, "Learn: %s", gdo_learn_state_to_string(status->learn));
            break;
        case GDO_CB_EVENT_OBSTRUCTION:
            ESP_LOGI(TAG, "Obstruction: %s", gdo_obstruction_state_to_string(status->obstruction));
            gdo->set_obstruction(status->obstruction);
            break;
        case GDO_CB_EVENT_MOTION:
            ESP_LOGI(TAG, "Motion: %s", gdo_motion_state_to_string(status->motion));
            gdo->set_motion_state(status->motion);
            break;
        case GDO_CB_EVENT_BATTERY:
            ESP_LOGI(TAG, "Battery: %s", gdo_battery_state_to_string(status->battery));
            break;
        case GDO_CB_EVENT_BUTTON:
            ESP_LOGI(TAG, "Button: %s", gdo_button_state_to_string(status->button));
            gdo->set_button_state(status->button);
            break;
        case GDO_CB_EVENT_MOTOR:
            ESP_LOGI(TAG, "Motor: %s", gdo_motor_state_to_string(status->motor));
            gdo->set_motor_state(status->motor);
            break;
        case GDO_CB_EVENT_OPENINGS:
            ESP_LOGI(TAG, "Openings: %d", status->openings);
            gdo->set_openings(status->openings);
            break;
        case GDO_CB_EVENT_TTC:
            ESP_LOGI(TAG, "Time to close: %d", status->ttc_seconds);
            break;
        case GDO_CB_EVENT_PAIRED_DEVICES:
            ESP_LOGI(TAG, "Paired devices: %d remotes, %d keypads, %d wall controls, %d accessories, %d total",
                    status->paired_devices.total_remotes, status->paired_devices.total_keypads,
                    status->paired_devices.total_wall_controls, status->paired_devices.total_accessories,
                    status->paired_devices.total_all);
            break;
        case GDO_CB_EVENT_OPEN_DURATION_MEASURMENT:
            ESP_LOGI(TAG, "Open duration: %d", status->open_ms);
            gdo->set_open_duration(status->open_ms);
            break;
        case GDO_CB_EVENT_CLOSE_DURATION_MEASURMENT:
            ESP_LOGI(TAG, "Close duration: %d", status->close_ms);
            gdo->set_close_duration(status->close_ms);
            break;
        default:
            ESP_LOGI(TAG, "Unknown event: %d", event);
            break;
        }
    }

    void GDOComponent::setup() {
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
            ESP_LOGI(TAG, "secplus GDO started!");
        } else {
            // check every 500ms for readiness before starting GDO
            this->set_interval("gdo_start", 500, [=]() {
                if (this->start_gdo_) {
                    gdo_start(gdo_event_handler, this);
                    ESP_LOGI(TAG, "secplus GDO started!");
                    this->cancel_interval("gdo_start");
                }
            });

        }
    }

    void GDOComponent::dump_config() {
        ESP_LOGCONFIG(TAG, "Setting up secplus GDO ...");
    }

} // namespace secplus_gdo
} // namespace esphome