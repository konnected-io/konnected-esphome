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

#include "esphome/components/cover/cover.h"
#include "esphome/core/component.h"
#include "automation.h"
#include "gdo.h"

namespace esphome {
namespace secplus_gdo {

using namespace esphome::cover;
    class GDODoor : public cover::Cover, public Component {
    public:
        cover::CoverTraits get_traits() override {
            auto traits = CoverTraits();
            traits.set_supports_stop(true);
            traits.set_supports_toggle(true);
            traits.set_supports_position(true);
            return traits;
        }

        void register_door_closing_warn_start_trigger(CoverClosingStartTrigger *trigger) {
            this->pre_close_start_trigger = trigger;
        }

        void register_door_closing_warn_end_trigger(CoverClosingEndTrigger *trigger) {
            this->pre_close_end_trigger = trigger;
        }

        void set_sync_state(bool synced) {
            this->synced_ = synced;
        }

        void do_action(const cover::CoverCall& call);
        void do_action_after_warning(const cover::CoverCall& call);
        void set_pre_close_warning_duration(uint32_t ms) { this->pre_close_duration_ = ms; }
        void set_toggle_only(bool val) { this->toggle_only_ = val; }
        void set_state(gdo_door_state_t state, float position);
        void cancel_pre_close_warning();

    protected:
        void control(const cover::CoverCall& call);

        CoverClosingStartTrigger *pre_close_start_trigger{nullptr};
        CoverClosingEndTrigger   *pre_close_end_trigger{nullptr};
        uint32_t                 pre_close_duration_{0};
        bool                     pre_close_active_{false};
        bool                     toggle_only_{false};
        optional<float>          target_position_{0};
        CoverOperation           prev_operation{COVER_OPERATION_IDLE};
        gdo_door_state_t         state_{GDO_DOOR_STATE_UNKNOWN};
        bool                     synced_{false};
    };
} // namespace secplus_gdo
} // namespace esphome
