#pragma once
#include "esphome/core/log.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/remote_base/nec_protocol.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
namespace nec_sealing_light {

static const char *const TAG = "nec_sealing_light";

class NecSealingLightComponent : public light::LightOutput {
 private:
  bool state_ = false;
  float brightness_ = 0.0f;
  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};

  void send_nec_(uint16_t address, uint16_t command) {
    auto call = this->transmitter_->transmit();
    remote_base::NECData data;
    data.address = address;
    data.command = command;
    data.command_repeats = 1;
    remote_base::NECProtocol().encode(call.get_data(), data);
    call.perform();
  }

 public:
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    this->transmitter_ = transmitter;
  }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }

  void write_state(light::LightState *light_state) override {
    float target_brightness;
    light_state->current_values.as_brightness(&target_brightness);

    if (target_brightness < 0.1f) {
      target_brightness = 0.05f;
    } else if (target_brightness < 0.3f) {
      target_brightness = 0.2f;
    } else if (target_brightness < 0.5f) {
      target_brightness = 0.4f;
    } else if (target_brightness < 0.7f) {
      target_brightness = 0.6f;
    } else if (target_brightness < 0.9f) {
      target_brightness = 0.8f;
    } else {
      target_brightness = 1.0f;
    }

    ESP_LOGD(TAG, "write_state: %.2f -> %.2f", this->brightness_, target_brightness);

    bool main_switch;
    light_state->current_values.as_binary(&main_switch);
    if (!main_switch) {
      if (this->state_)
        command_off_();
      return;
    }

    if (target_brightness >= 0.9f) {
      if (!this->state_ || this->brightness_ < 0.9f)
        command_full_light_();
      return;
    }
    if (target_brightness < 0.1f) {
      if (!this->state_ || this->brightness_ >= 0.1f)
        command_night_light_();
      return;
    }

    // Already at target — nothing to send
    if (this->state_ && this->brightness_ == target_brightness)
      return;

    if (!this->state_ || this->brightness_ < 0.1f) {
      command_full_light_();
      delay(500);
    }
    for (int i = 0; i < 10 && this->brightness_ < target_brightness; i++) {
      command_up_();
      if (this->brightness_ < target_brightness)
        delay(500);
    }
    for (int i = 0; i < 10 && this->brightness_ > target_brightness; i++) {
      command_down_();
      if (this->brightness_ > target_brightness)
        delay(500);
    }
  }

 protected:
  void command_full_light_() {
    this->send_nec_(0x6D82, 0x59A6);
    this->state_ = true;
    this->brightness_ = 1.0f;
  }

  void command_off_() {
    this->send_nec_(0x6D82, 0x41BE);
    this->state_ = false;
    this->brightness_ = 0.0f;
  }

  void command_night_light_() {
    this->send_nec_(0x6D82, 0x43BC);
    this->state_ = true;
    this->brightness_ = 0.05f;
  }

  void command_up_() {
    this->send_nec_(0x6D82, 0x45BA);
    if (this->brightness_ < 0.3f) {
      this->brightness_ = 0.4f;
    } else if (this->brightness_ < 0.5f) {
      this->brightness_ = 0.6f;
    } else if (this->brightness_ < 0.7f) {
      this->brightness_ = 0.8f;
    } else if (this->brightness_ < 0.9f) {
      this->brightness_ = 1.0f;
    }
  }

  void command_down_() {
    this->send_nec_(0x6D82, 0x44BB);
    if (this->brightness_ > 0.9f) {
      this->brightness_ = 0.8f;
    } else if (this->brightness_ > 0.7f) {
      this->brightness_ = 0.6f;
    } else if (this->brightness_ > 0.5f) {
      this->brightness_ = 0.4f;
    } else if (this->brightness_ > 0.3f) {
      this->brightness_ = 0.2f;
    }
  }
};

}  // namespace nec_sealing_light
}  // namespace esphome
