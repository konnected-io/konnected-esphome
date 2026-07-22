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
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace secplus_gdo {

class GDOBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  // Event-only sensor types (button, motion, motor) receive state exclusively
  // from GDO events and would otherwise remain stateless (unavailable in Home
  // Assistant) after every boot until their first physical event occurs.
  // Seed those with an initial OFF at setup; a real event overrides it.
  void set_event_only(bool event_only) { this->event_only_ = event_only; }
  void setup() override {
    if (this->event_only_ && !this->has_state()) {
      this->publish_initial_state(false);
    }
  }

 protected:
  bool event_only_{false};
};

} // namespace secplus_gdo
} // namespace esphome