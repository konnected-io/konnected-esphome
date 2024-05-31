/************************************
 *
 * Copyright (C) 2024 Konnected.io
 *
 * GNU GENERAL PUBLIC LICENSE
 ************************************/

#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "gdo.h"

namespace esphome {
namespace secplus_gdo {

    enum SwitchType {
        LEARN,
        TOGGLE_ONLY,
    };

    class GDOSwitch : public switch_::Switch, public Component {
    public:
        void dump_config() override {}
        void setup() override {
            bool value = false;
            this->pref_ = global_preferences->make_preference<bool>(this->get_object_id_hash());
            if (!this->pref_.load(&value)) {
                value = false;
            }

            this->write_state(value);
        }

        void write_state(bool state) override {
            if (state == this->state) {
                return;
            }

            if (this->type_ == SwitchType::TOGGLE_ONLY) {
                if (this->f_control) {
                    this->f_control(state);
                    this->pref_.save(&state);
                }
            }

            if (this->type_ == SwitchType::LEARN) {
                if (state) {
                    gdo_activate_learn();
                } else {
                    gdo_deactivate_learn();
                }
            }

            this->publish_state(state);
        }

        void set_type(SwitchType type) { this->type_ = type; }
        void set_control_function(std::function<void(bool)> f) { f_control = f; }

    protected:
        SwitchType                type_{SwitchType::LEARN};
        std::function<void(bool)> f_control{nullptr};
        ESPPreferenceObject       pref_;
    };

} // namespace secplus_gdo
} // namespace esphome
