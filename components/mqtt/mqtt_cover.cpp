#include "mqtt_cover.h"
#include "esphome/core/log.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_COVER

namespace esphome::mqtt {

static const char *const TAG = "mqtt.cover";

using namespace esphome::cover;

MQTTCoverComponent::MQTTCoverComponent(Cover *cover) : cover_(cover) {}
void MQTTCoverComponent::setup() {
  auto traits = this->cover_->get_traits();
  this->cover_->add_on_state_callback([this]() { this->publish_state(); });
  this->subscribe(this->get_command_topic_(), [this](const std::string &topic, const std::string &payload) {
    auto call = this->cover_->make_call();
    call.set_command(payload.c_str());
    call.perform();
  });
  if (traits.get_supports_position()) {
    this->subscribe(this->get_position_command_topic(), [this](const std::string &topic, const std::string &payload) {
      auto value = parse_number<float>(payload);
      if (!value.has_value()) {
        ESP_LOGW(TAG, "Invalid position value: '%s'", payload.c_str());
        return;
      }
      auto call = this->cover_->make_call();
      call.set_position(*value / 100.0f);
      call.perform();
    });
  }
  if (traits.get_supports_tilt()) {
    this->subscribe(this->get_tilt_command_topic(), [this](const std::string &topic, const std::string &payload) {
      auto value = parse_number<float>(payload);
      if (!value.has_value()) {
        ESP_LOGW(TAG, "Invalid tilt value: '%s'", payload.c_str());
        return;
      }
      auto call = this->cover_->make_call();
      call.set_tilt(*value / 100.0f);
      call.perform();
    });
  }
}

void MQTTCoverComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT cover '%s':", this->cover_->get_name().c_str());
  auto traits = this->cover_->get_traits();
  bool has_command_topic = traits.get_supports_position() || !traits.get_supports_tilt();
  LOG_MQTT_COMPONENT(true, has_command_topic)
  if (traits.get_supports_position()) {
    ESP_LOGCONFIG(TAG,
                  "  Position State Topic: '%s'\n"
                  "  Position Command Topic: '%s'",
                  this->get_position_state_topic().c_str(), this->get_position_command_topic().c_str());
  }
  if (traits.get_supports_tilt()) {
    ESP_LOGCONFIG(TAG,
                  "  Tilt State Topic: '%s'\n"
                  "  Tilt Command Topic: '%s'",
                  this->get_tilt_state_topic().c_str(), this->get_tilt_command_topic().c_str());
  }
}
void MQTTCoverComponent::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  const auto device_class = this->cover_->get_device_class_ref();
  if (!device_class.empty()) {
    root[MQTT_DEVICE_CLASS] = device_class;
  }
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

  auto traits = this->cover_->get_traits();
  if (traits.get_is_assumed_state()) {
    root[MQTT_OPTIMISTIC] = true;
  }
  if (traits.get_supports_position()) {
    root[MQTT_POSITION_TOPIC] = this->get_position_state_topic();
    root[MQTT_SET_POSITION_TOPIC] = this->get_position_command_topic();
  }
  if (traits.get_supports_tilt()) {
    root[MQTT_TILT_STATUS_TOPIC] = this->get_tilt_state_topic();
    root[MQTT_TILT_COMMAND_TOPIC] = this->get_tilt_command_topic();
  }
  if (traits.get_supports_tilt() && !traits.get_supports_position()) {
    config.command_topic = false;
  }
}

MQTT_COMPONENT_TYPE(MQTTCoverComponent, "cover")
const EntityBase *MQTTCoverComponent::get_entity() const { return this->cover_; }

bool MQTTCoverComponent::send_initial_state() { return this->publish_state(); }
bool MQTTCoverComponent::publish_state() {
  auto traits = this->cover_->get_traits();
  const char *state_s = this->cover_->current_operation == COVER_OPERATION_OPENING   ? "opening"
                        : this->cover_->current_operation == COVER_OPERATION_CLOSING ? "closing"
                        : this->cover_->position == COVER_CLOSED                     ? "closed"
                        : this->cover_->position == COVER_OPEN                       ? "open"
                        : traits.get_supports_position()                             ? "open"
                                                                                     : "unknown";

  return this->publish_json(this->get_state_topic_(), [this, traits, state_s](JsonObject root) {
    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
    root[ESPHOME_F("state")] = state_s;
    if (traits.get_supports_position()) {
      root[ESPHOME_F("position")] = static_cast<int>(roundf(this->cover_->position * 100));
    }
    if (traits.get_supports_tilt()) {
      root[ESPHOME_F("tilt")] = static_cast<int>(roundf(this->cover_->tilt * 100));
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
  });
}

}  // namespace mqtt
}  // namespace esphome

#endif
#endif  // USE_MQTT
