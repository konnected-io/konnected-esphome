/************************************
 *
 * Copyright (C) 2024 Konnected.io
 *
 * GNU GENERAL PUBLIC LICENSE
 ************************************/

#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "../secplus_gdo.h"
#include "gdo.h"

namespace esphome {
namespace secplus_gdo {

    class GDOSelect : public select::Select, public Component {
    public:
        void setup() override {
            std::string value;
            size_t index;
            this->pref_ = global_preferences->make_preference<size_t>(this->get_object_id_hash());
            if (!this->pref_.load(&index)) {
                value = this->initial_option_;
            } else if (!this->has_index(index)) {
                value = this->initial_option_;
            } else {
                value = this->at(index).value();
            }

            this->control(value);
        }

        void set_initial_option(const std::string &initial_option) { this->initial_option_ = initial_option; }

        void update_state(gdo_protocol_type_t protocol) {
            if (this->has_index(protocol)) {
                std::string value = this->at(protocol).value();
                if (this->has_state() && value != this->state) {
                    this->pref_.save(&protocol);
                }

                this->publish_state(value);
            }
        }

    protected:
        void control(const std::string &value) override {
            auto idx = this->index_of(value);
            if (idx.has_value()) {
                gdo_protocol_type_t protocol = static_cast<gdo_protocol_type_t>(idx.value());
                if (gdo_set_protocol(protocol) == ESP_OK) {
                    this->update_state(protocol);
                }
            }
        }

        std::string initial_option_;
        ESPPreferenceObject pref_;
    };

} // namespace secplus_gdo
} // namespace esphome
