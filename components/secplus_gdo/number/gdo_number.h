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

# pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace secplus_gdo {

class GDONumber : public number::Number, public Component {
    public:
        void dump_config() override {}
        void setup() override {
            float value;
            this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
            if (!this->pref_.load(&value)) {
                value = 0;
            }

            this->control(value);
        }

        void update_state(float value) {
            if (value == this->state) {
                return;
            }

            this->state = value;
            this->publish_state(value);
            this->pref_.save(&value);
        }

        void control(float value) override {
            if (value == this->state) {
                return;
            }

            if (this->f_control) {
                this->f_control(value);
                this->update_state(value);
            }
        }

        void set_control_function(std::function<int(float)> f) { f_control = f; }

    protected:
        ESPPreferenceObject pref_;
        std::function<int(float)> f_control{nullptr};
    };
} // namespace secplus_gdo
} // namespace esphome