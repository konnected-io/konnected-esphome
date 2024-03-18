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
#include "number/gdo_number.h"
#include "esphome/core/defines.h"
#include "select/gdo_select.h"
#include "gdolib.h"

namespace esphome {
namespace secplus_gdo {
    class GDOComponent : public Component {
    public:
        void setup() override;
        void loop() override {};
        void dump_config() override;
        // Use Late priority so we do not start the GDO lib until all saved preferences are loaded
        float get_setup_priority() const override { return setup_priority::LATE; }

        void register_protocol_select(GDOSelect *select) { this->protocol_select_ = select; }
        void set_protocol_state(gdo_protocol_type_t protocol) { if (this->protocol_select_) { this->protocol_select_->update_state(protocol); } }

        void register_motion(std::function<void(bool)> f) { f_motion = f; }
        void set_motion_state(gdo_motion_state_t state) { if (f_motion) { f_motion(state == GDO_MOTION_STATE_DETECTED); } }

        void register_obstruction(std::function<void(bool)> f) { f_obstruction = f; }
        void set_obstruction(gdo_obstruction_state_t state) {
            if (f_obstruction) { f_obstruction(state == GDO_OBSTRUCTION_STATE_OBSTRUCTED); }
        }

        void register_button(std::function<void(bool)> f) { f_button = f; }
        void set_button_state(gdo_button_state_t state) { if (f_button) { f_button(state == GDO_BUTTON_STATE_PRESSED); } }

        void register_motor(std::function<void(bool)> f) { f_motor = f; }
        void set_motor_state(gdo_motor_state_t state) { if (f_motor) { f_motor(state == GDO_MOTOR_STATE_ON); } }

        void register_openings(std::function<void(uint16_t)> f) { f_openings = f; }
        void set_openings(uint16_t openings) { if (f_openings) { f_openings(openings); } }

        void register_door(std::function<void(gdo_door_state_t, float)> f) { f_door = f; }
        void set_door_state(gdo_door_state_t state, float position) { if (f_door) { f_door(state, position); } }

        void register_light(std::function<void(gdo_light_state_t)> f) { f_light = f; }
        void set_light_state(gdo_light_state_t state) { if (f_light) { f_light(state); } }

        void register_lock(std::function<void(gdo_lock_state_t)> f) { f_lock = f; }
        void set_lock_state(gdo_lock_state_t state) { if (f_lock) { f_lock(state); } }

        void register_learn(std::function<void(bool)> f) { f_learn = f; }
        void set_learn_state(gdo_learn_state_t state) { if (f_learn) { f_learn(state == GDO_LEARN_STATE_ACTIVE); } }

        void register_open_duration(GDONumber* num) { open_duration_ = num; }
        void set_open_duration(uint16_t ms ) { if (open_duration_) { open_duration_->update_state(ms); } }

        void register_close_duration(GDONumber* num) { close_duration_ = num; }
        void set_close_duration(uint16_t ms ) { if (close_duration_) { close_duration_->update_state(ms); } }

    protected:
        gdo_status_t status_;
        std::function<void(gdo_door_state_t, float)> f_door{nullptr};
        std::function<void(gdo_lock_state_t)>        f_lock{nullptr};
        std::function<void(gdo_light_state_t)>       f_light{nullptr};
        std::function<void(uint16_t)>                f_openings{nullptr};
        std::function<void(bool)>                    f_motion{nullptr};
        std::function<void(bool)>                    f_obstruction{nullptr};
        std::function<void(bool)>                    f_button{nullptr};
        std::function<void(bool)>                    f_motor{nullptr};
        std::function<void(bool)>                    f_learn{nullptr};
        GDONumber*                                   open_duration_{nullptr};
        GDONumber*                                   close_duration_{nullptr};
        GDOSelect*                                   protocol_select_{nullptr};

    }; // GDOComponent
} // namespace secplus_gdo
} // namespace esphome
