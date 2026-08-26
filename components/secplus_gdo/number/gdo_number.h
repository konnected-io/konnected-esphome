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

#include <cmath>
#include <cstdint>
#include <functional>

namespace esphome {
namespace secplus_gdo {

/* Every value this platform carries is a 32-bit integer: the Security+ client
 * ID, the rolling code and the door travel times. IEEE-754 binary32 only
 * represents integers exactly up to 2^24, so a float backing store silently
 * rounds anything larger -- a client ID of 0x7F7F2908 comes back as
 * 0x7F7F2900, losing the 0x2908 suffix that the re-sync generator deliberately
 * puts there. Keep an integer as the authoritative value and use the Number's
 * float `state` only for the ESPHome entity API.
 */
class GDONumber : public number::Number, public Component {
    public:
        void dump_config() override {}

        void setup() override {
            uint32_t value = 0;
            this->pref_ = this->make_entity_preference<uint32_t>(PREF_VERSION);
            bool restored = this->pref_.load(&value);

            if (!restored) {
                // Adopt the value written by firmware that stored a float. Both
                // stores are 4 bytes, so PREF_VERSION keeps them in separate keys.
                auto legacy_pref = this->make_entity_preference<float>();
                float legacy;
                if (legacy_pref.load(&legacy) && legacy >= 0.0f) { // also rejects NaN
                    value = to_u32(legacy);
                    restored = true;
                    this->pref_.save(&value);
                }
            }

            if (!restored) {
                // Nothing was ever saved for this entity; leave the GDO library
                // defaults (client ID 0x2908, rolling code 0) in place.
                return;
            }

            this->value_ = value;
            if (this->f_control) {
                this->f_control(value);
            }
            this->publish_state((float)value);
        }

        /* Report a value observed on the wire: persist and publish it, but do
         * not push it back down to the GDO library.
         */
        void update_state(uint32_t value) {
            if (this->has_state() && value == this->value_) {
                return;
            }

            this->value_ = value;
            this->pref_.save(&value);
            this->publish_state((float)value);
        }

        void control(float value) override {
            if (!(value >= 0.0f)) { // also rejects NaN
                return;
            }

            uint32_t new_value = to_u32(value);
            if (this->has_state() && new_value == this->value_) {
                return;
            }

            if (this->f_control) {
                this->f_control(new_value);
                this->update_state(new_value);
            }
        }

        void set_control_function(std::function<int(uint32_t)> f) { f_control = f; }

    protected:
        // XORed into the preference key so the uint32_t store is never read back
        // out of the same-sized float store written by earlier firmware.
        static constexpr uint32_t PREF_VERSION = 1;

        // Saturating float -> uint32_t; the caller has already excluded NaN and
        // negatives, and 2^32 is not representable as a uint32_t.
        static uint32_t to_u32(float value) {
            return value >= 4294967296.0f ? UINT32_MAX : (uint32_t)llroundf(value);
        }

        ESPPreferenceObject pref_;
        uint32_t value_{0};
        std::function<int(uint32_t)> f_control{nullptr};
    };
} // namespace secplus_gdo
} // namespace esphome
