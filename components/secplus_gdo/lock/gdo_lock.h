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

#include "esphome/components/lock/lock.h"
#include "esphome/core/component.h"
#include "../gdo.h"

namespace esphome {
namespace secplus_gdo {

    class GDOLock : public lock::Lock, public Component {
        public:
        void set_state(gdo_lock_state_t state) {
            if (state == GDO_LOCK_STATE_LOCKED && this->state == lock::LockState::LOCK_STATE_LOCKED) {
                return;
            }
            if (state == GDO_LOCK_STATE_UNLOCKED && this->state == lock::LockState::LOCK_STATE_UNLOCKED) {
                return;
            }

            auto call = this->make_call();
            if (state == GDO_LOCK_STATE_LOCKED) {
                call.set_state(lock::LockState::LOCK_STATE_LOCKED);
            } else if (state == GDO_LOCK_STATE_UNLOCKED) {
                call.set_state(lock::LockState::LOCK_STATE_UNLOCKED);
            }
            this->control(call);
        }

        void control(const lock::LockCall& call) override {
            auto state = *call.get_state();

            if (state == lock::LockState::LOCK_STATE_LOCKED) {
                gdo_lock();
            } else if (state == lock::LockState::LOCK_STATE_UNLOCKED) {
                gdo_unlock();
            }

            this->publish_state(state);
        }
    };

} // namespace secplus_gdo
} // namespace esphome
