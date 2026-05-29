#include "esphome.h"

class NecSealingLightComponent : public Component, public LightOutput {
 private:
  bool state = false;
  float brightness = 0.0f;
 public:
  
  void setup() override {
  }
  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::ON_OFF, ColorMode::BRIGHTNESS});
    return traits;
  }
  void update_state(LightState *state) override {
  }
  void write_state(LightState *state) override {
    float brightnessB;
    state->current_values.as_brightness(&brightnessB);
    if (brightnessB < 0.1f) {
      brightnessB = 0.05f;
    } else if (brightnessB < 0.3f) {
      brightnessB = 0.2f;
    } else if (brightnessB < 0.5f) {
      brightnessB = 0.4f;
    } else if (brightnessB < 0.7f) {
      brightnessB = 0.6f;
    } else if (brightnessB < 0.9f) {
      brightnessB = 0.8f;
    } else {
      brightnessB = 1.0f;
    }
    ESP_LOGD("NecSealingLightCmponent", "write_state: %f -> %f", this->brightness, brightnessB);

    bool mainSwitch;
    state->current_values.as_binary(&mainSwitch);
    if (! mainSwitch) {
      commandOff(state);
      return;
    }

    if (brightnessB >= 0.9f) {
      commandFullLight(state);
      return;
    }
    if (brightnessB < 0.1f) {
      commandNightLight(state);
      return;
    }

    if (! this->state || this->brightness < 0.1f) {
      commandFullLight(state);
      delay(500);
    }
    for (int i = 0; i < 10 && this->brightness < brightnessB; i++) {
      commandUp(state);
      if (this->brightness < brightnessB) {
        delay(500);
      }
    }
    for (int i = 0; i < 10 && this->brightness > brightnessB; i++) {
      commandDown(state);
      if (this->brightness > brightnessB) {
        delay(500);
      }
    }
  }

  void commandFullLight(LightState *state)
  {
    remote_base::NECAction<> *remote_base_necaction = new remote_base::NECAction<>();
    remote_base_necaction->set_transmitter(::irsend);
    remote_base_necaction->set_address(0x6D82);
    remote_base_necaction->set_command(0x59A6);
    remote_base_necaction->set_command_repeats(1);
    remote_base_necaction->play_complex();
    this->state = true;
    this->brightness = 1.0f;
  }

  void commandOff(LightState *state)
  {
    remote_base::NECAction<> *remote_base_necaction = new remote_base::NECAction<>();
    remote_base_necaction->set_transmitter(::irsend);
    remote_base_necaction->set_address(0x6D82);
    remote_base_necaction->set_command(0x41BE);
    remote_base_necaction->set_command_repeats(1);
    remote_base_necaction->play_complex();
    this->state = false;
    this->brightness = 0.0f;
  }

  void commandNightLight(LightState *state)
  {
    remote_base::NECAction<> *remote_base_necaction = new remote_base::NECAction<>();
    remote_base_necaction->set_transmitter(::irsend);
    remote_base_necaction->set_address(0x6D82);
    remote_base_necaction->set_command(0x43BC);
    remote_base_necaction->set_command_repeats(1);
    remote_base_necaction->play_complex();
    this->state = true;
    this->brightness = 0.05f;
  }

  void commandUp(LightState *state)
  {
    remote_base::NECAction<> *remote_base_necaction = new remote_base::NECAction<>();
    remote_base_necaction->set_transmitter(::irsend);
    remote_base_necaction->set_address(0x6D82);
    remote_base_necaction->set_command(0x45BA);
    remote_base_necaction->set_command_repeats(1);
    remote_base_necaction->play_complex();
    if (this->brightness < 0.3f) {
      this->brightness = 0.4f;
    } else if (this->brightness < 0.5f) {
      this->brightness = 0.6f;
    } else if (this->brightness < 0.7f) {
      this->brightness = 0.8f;
    } else if (this->brightness < 0.9f) {
      this->brightness = 1.0f;
    }
  }

  void commandDown(LightState *state)
  {
    remote_base::NECAction<> *remote_base_necaction = new remote_base::NECAction<>();
    remote_base_necaction->set_transmitter(::irsend);
    remote_base_necaction->set_address(0x6D82);
    remote_base_necaction->set_command(0x44BB);
    remote_base_necaction->set_command_repeats(1);
    remote_base_necaction->play_complex();
    if (this->brightness > 0.9f) {
      this->brightness = 0.8f;
    } else if (this->brightness > 0.7f) {
      this->brightness = 0.6f;
    } else if (this->brightness > 0.5f) {
      this->brightness = 0.4f;
    } else if (this->brightness > 0.3f) {
      this->brightness = 0.2f;
    }
  }
};