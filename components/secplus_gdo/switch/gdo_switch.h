/************************************
 *
 * Copyright (C) 2024 Konnected.io
 *
 * GNU GENERAL PUBLIC LICENSE
 ************************************/

#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "../gdo.h"

namespace esphome {
namespace secplus_gdo {

    class GDOSwitch : public switch_::Switch, public Component {
    public:
        void write_state(bool state) override {
            if (state) {
                gdo_activate_learn();
            } else {
                gdo_deactivate_learn();
            }
        }
    };

} // namespace secplus_gdo
} // namespace esphome
