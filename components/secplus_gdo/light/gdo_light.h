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

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary/light/binary_light_output.h"
#include "gdo.h"

namespace esphome {
namespace secplus_gdo {

class GDOLight : public binary::BinaryLightOutput, public Component {
    public:
        void setup_state(light::LightState *state) override { this->state_ = state; }

        void write_state(light::LightState *state) override {
            bool binary;
            state->current_values_as_binary(&binary);
            if (binary)
                gdo_light_on();
            else
                gdo_light_off();
        }

        void set_state(gdo_light_state_t state) {
            bool is_on = state == GDO_LIGHT_STATE_ON;
            this->state_->current_values.set_state(is_on);
            this->state_->remote_values.set_state(is_on);
            this->state_->publish_state();
        }

    private:
        light::LightState *state_{nullptr};
    }; // GDOLight
} // namespace secplus_gdo
} // namespace esphome
