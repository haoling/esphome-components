#include "daikin_arc472a66.h"

#include <cmath>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace daikin_arc472a66 {

static const char *const TAG = "daikin_arc472a66.climate";

void DaikinArc472A66Climate::setup() {
  climate_ir::ClimateIR::setup();

  this->set_supported_custom_fan_modes({"1", "2", "3", "4", "5"});

  if (std::isnan(this->target_humidity))
    this->target_humidity = 0;
  if (std::isnan(this->current_temperature))
    this->current_temperature = 0;
  if (std::isnan(this->current_humidity))
    this->current_humidity = 0;
}

// climate::ClimateMode → ダイキンのモードビット (電源ビットは含まない)
uint8_t DaikinArc472A66Climate::mode_bits_for_(climate::ClimateMode mode) {
  switch (mode) {
    case climate::CLIMATE_MODE_COOL:
      return DAIKIN_MODE_COOL;
    case climate::CLIMATE_MODE_DRY:
      return DAIKIN_MODE_DRY;
    case climate::CLIMATE_MODE_HEAT:
      return DAIKIN_MODE_HEAT;
    case climate::CLIMATE_MODE_HEAT_COOL:
      return DAIKIN_MODE_AUTO;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return DAIKIN_MODE_FAN;
    default:
      return DAIKIN_MODE_AUTO;
  }
}

uint8_t DaikinArc472A66Climate::operation_mode_() {
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    // 実測: 純正リモコンは停止時も直前モードのビットを残し、電源ビット
    // (DAIKIN_MODE_ON = 0x01) だけを落とす。呼び出し側 (transmit_state) が
    // 最後に | 0x08 するので、ここでは電源ビットを付けない。
    return this->mode_bits_for_(this->last_active_mode_);
  }

  uint8_t operating_mode = DAIKIN_MODE_ON;
  operating_mode |= this->mode_bits_for_(this->mode);
  return operating_mode;
}

uint16_t DaikinArc472A66Climate::fan_speed_() {
  uint8_t speed_byte;
  if (this->has_custom_fan_mode()) {
    auto custom = this->get_custom_fan_mode();
    if (custom == "1") {
      speed_byte = DAIKIN_FAN_1;
    } else if (custom == "2") {
      speed_byte = DAIKIN_FAN_2;
    } else if (custom == "3") {
      speed_byte = DAIKIN_FAN_3;
    } else if (custom == "4") {
      speed_byte = DAIKIN_FAN_4;
    } else if (custom == "5") {
      speed_byte = DAIKIN_FAN_5;
    } else {
      speed_byte = DAIKIN_FAN_AUTO;
    }
  } else if (this->fan_mode.value_or(climate::CLIMATE_FAN_AUTO) == climate::CLIMATE_FAN_QUIET) {
    speed_byte = DAIKIN_FAN_SILENT;
  } else {
    speed_byte = DAIKIN_FAN_AUTO;
  }

  // 実測: byte[8]の下位nibble・byte[9]はスイング用ではなく常に0x00
  // (本家daikin_arcのswing_mode→ビットOR方式はARC472A66では機能しない。
  //  風向はheader frame側で別途扱う。2-5節参照)
  return static_cast<uint16_t>(speed_byte) << 8;
}

uint8_t DaikinArc472A66Climate::temperature_() {
  // 実測: 停止時も「直前モード」用の温度エンコード方式がそのまま使われる
  // (例: 直前がドライだと停止フレームの温度も 0xc0 になる)。
  climate::ClimateMode effective_mode =
      (this->mode == climate::CLIMATE_MODE_OFF) ? this->last_active_mode_ : this->mode;
  switch (effective_mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
      return 0x32;
    case climate::CLIMATE_MODE_DRY:
      return 0xc0;
    case climate::CLIMATE_MODE_HEAT_COOL: {
      // 快適エコ自動: 絶対温度ではなく「適温からの相対オフセット」。
      // 式(実測確定、2-2節): n>=0 は 0xC0+n*2、n<0 は 0xE0+n*2 (n:0.5℃刻み)
      float offset = clamp<float>(this->comfort_offset_, DAIKIN_COMFORT_OFFSET_MIN, DAIKIN_COMFORT_OFFSET_MAX);
      int units = (int) std::lround(offset * 2.0f);
      return (offset >= 0) ? (uint8_t) (0xC0 + units) : (uint8_t) (0xE0 + units);
    }
    default: {
      float new_temp = clamp<float>(this->target_temperature, DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX);
      uint8_t temperature = (uint8_t) std::floor(new_temp);
      return temperature << 1 | (new_temp - temperature > 0 ? 0x01 : 0);
    }
  }
}

uint8_t DaikinArc472A66Climate::humidity_() {
  // 実測: 停止時も「直前モード」を参照する (temperature_()と同じ理由)
  climate::ClimateMode effective_mode =
      (this->mode == climate::CLIMATE_MODE_OFF) ? this->last_active_mode_ : this->mode;
  if (effective_mode == climate::CLIMATE_MODE_DRY) {
    // 実測: 生パーセント値をそのままエンコードするだけ (オフセットなし)
    float target = std::isnan(this->target_humidity) ? DAIKIN_HUMIDITY_MIN : this->target_humidity;
    return (uint8_t) clamp<float>(std::round(target), DAIKIN_HUMIDITY_MIN, DAIKIN_HUMIDITY_MAX);
  }
  if (effective_mode == climate::CLIMATE_MODE_HEAT) {
    // 実測: 暖房モードの「加湿」ボタンは byte[7] を 0xff にするだけ
    return this->humidify_ ? 0xff : 0x00;
  }
  return 0x00;
}

uint8_t DaikinArc472A66Climate::vertical_vane_byte_() {
  // センサー風向ON時は、header側の上下・左右を自動値に強制する (実測確認済み)
  if (this->sensor_vane_ != SensorVane::OFF)
    return static_cast<uint8_t>(VerticalVane::AUTO);
  return static_cast<uint8_t>(this->vertical_vane_);
}

uint8_t DaikinArc472A66Climate::horizontal_vane_byte_() {
  if (this->sensor_vane_ != SensorVane::OFF)
    return static_cast<uint8_t>(HorizontalVane::AUTO);
  return static_cast<uint8_t>(this->horizontal_vane_);
}

climate::ClimateTraits DaikinArc472A66Climate::traits() {
  climate::ClimateTraits traits = climate_ir::ClimateIR::traits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_TARGET_HUMIDITY);
  traits.set_visual_min_humidity(DAIKIN_HUMIDITY_MIN);
  traits.set_visual_max_humidity(DAIKIN_HUMIDITY_MAX);
  return traits;
}

void DaikinArc472A66Climate::transmit_state() {
  // 停止(OFF)を送る前に、直前のモードを覚えておく。
  // (実測により、ARC472A66純正リモコンは停止フレームでも直前モードの
  //  ビットを残していることが判明したため)
  if (this->mode != climate::CLIMATE_MODE_OFF) {
    this->last_active_mode_ = this->mode;
  }

  bool powered_on = this->mode != climate::CLIMATE_MODE_OFF;

  // ---- header frame (20バイト) ----
  // 本家daikin_arcは固定値のみ送っていたが、実測の結果、風向上下/左右・
  // サーキュレーション・Streamer(空清)・電源状態はこちら側で表現されている
  // ことが判明したため、動的に組み立てる (3章・2-5節参照)。
  uint8_t remote_header[DAIKIN_HEADER_FRAME_SIZE] = {0x11, 0xDA, 0x27, 0x00, 0x02, 0x00, 0x44, 0x02, 0x20, 0x0e,
                                                      0x83, 0xE6, 0xE6, 0x3e, 0xD2, 0xf0, 0x00, 0x36, 0x00, 0x00};

  remote_header[6] = powered_on ? 0x44 : 0xC4;
  // 実測: 風向上下/左右ボタンを一度でも操作すると 0x0e→0x16 に切り替わり
  // そのまま維持される。停止時は 0x02。
  remote_header[9] = powered_on ? (this->vane_adjusted_ ? 0x16 : 0x0e) : 0x02;
  remote_header[11] = this->vertical_vane_byte_();
  remote_header[12] = this->circulation_ ? 0xd6 : 0xe6;
  remote_header[13] = this->horizontal_vane_byte_();
  remote_header[14] = this->streamer_ ? 0xd2 : 0xc2;

  // ---- state frame (19バイト) ----
  uint8_t remote_state[DAIKIN_STATE_FRAME_SIZE] = {0x11, 0xDA, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                    0x00, 0x06, 0x60, 0x00, 0x0a, 0xC4, 0x80, 0x2a, 0x00};

  remote_state[5] = this->operation_mode_() | 0x08;
  remote_state[6] = this->temperature_();
  remote_state[7] = this->humidity_();
  if (remote_state[7] != this->last_humidity_ && this->mode != climate::CLIMATE_MODE_OFF) {
    ESP_LOGD(TAG, "Set Humidity: %d, %d", (int) this->target_humidity, (int) remote_state[7]);
    this->last_humidity_ = remote_state[7];
  }
  uint16_t fan_speed = this->fan_speed_();
  remote_state[8] = fan_speed >> 8;
  remote_state[9] = fan_speed & 0xff;

  // パワフル・おやすみ (実測確認済み、独立ビット。preset経由で択一制御)
  uint8_t special_flags = 0x00;
  if (this->preset == climate::CLIMATE_PRESET_BOOST) {
    special_flags |= DAIKIN_POWERFUL_BIT_MASK;
  } else if (this->preset == climate::CLIMATE_PRESET_SLEEP) {
    special_flags |= DAIKIN_SLEEP_BIT_MASK;
  }
  remote_state[13] = special_flags;

  // 快適エコ自動⇔ビズ自動トグル (実測確認済み。快適エコ自動モード以外は常に0x0a)
  remote_state[14] = (this->mode == climate::CLIMATE_MODE_HEAT_COOL && this->comfort_mode_ == ComfortBiasMode::BIZ)
                          ? DAIKIN_BIZ_AUTO_BYTE
                          : DAIKIN_COMFORT_AUTO_BYTE;

  // センサー風向ボタン (実測確認済み。OFF以外はheader側も自動値に強制される)
  remote_state[16] = static_cast<uint8_t>(this->sensor_vane_);

  // Calculate checksum
  for (size_t i = 0; i < sizeof(remote_header) - 1; i++) {
    remote_header[sizeof(remote_header) - 1] += remote_header[i];
  }
  for (int i = 0; i < DAIKIN_STATE_FRAME_SIZE - 1; i++) {
    remote_state[DAIKIN_STATE_FRAME_SIZE - 1] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_IR_FREQUENCY);

  data->mark(DAIKIN_ARC_PRE_MARK);
  data->space(DAIKIN_ARC_PRE_SPACE);

  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);

  for (uint8_t i : remote_header) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {
      data->mark(DAIKIN_BIT_MARK);
      bool bit = i & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(DAIKIN_MESSAGE_SPACE);

  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);

  for (uint8_t i : remote_state) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {
      data->mark(DAIKIN_BIT_MARK);
      bool bit = i & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(0);

  transmit.perform();

  // 実測: 内部クリーンは停止フレームへのビット追加ではなく、専用の8バイト
  // 特殊フレームを停止フレーム送信直後に追加送信する方式が実機に忠実 (2-6節)
  if (this->mode == climate::CLIMATE_MODE_OFF && this->power_off_internal_clean_) {
    this->send_special_command_(DAIKIN_SPECIAL_CMD_INTERNAL_CLEAN);
  }
}

void DaikinArc472A66Climate::send_special_command_(uint8_t sub_code, uint8_t payload) {
  // 内部クリーン・フィルター掃除・お知らせ用の8バイト短縮フレーム
  // (`11 DA 27 00 84 <sub> <payload> <checksum>`, 2-6節参照)
  uint8_t frame[DAIKIN_SPECIAL_FRAME_SIZE] = {0x11, 0xDA, 0x27, 0x00, 0x84, sub_code, payload, 0x00};

  for (size_t i = 0; i < sizeof(frame) - 1; i++) {
    frame[sizeof(frame) - 1] += frame[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_IR_FREQUENCY);

  data->mark(DAIKIN_ARC_PRE_MARK);
  data->space(DAIKIN_ARC_PRE_SPACE);

  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);

  for (uint8_t i : frame) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {
      data->mark(DAIKIN_BIT_MARK);
      bool bit = i & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(0);

  transmit.perform();
}

void DaikinArc472A66Climate::set_comfort_mode(ComfortBiasMode mode) {
  this->comfort_mode_ = mode;
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_comfort_offset(float offset_celsius) {
  this->comfort_offset_ = clamp<float>(offset_celsius, DAIKIN_COMFORT_OFFSET_MIN, DAIKIN_COMFORT_OFFSET_MAX);
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_humidify(bool enabled) {
  this->humidify_ = enabled;
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_vertical_vane(VerticalVane vane) {
  this->vertical_vane_ = vane;
  this->vane_adjusted_ = true;
  this->swing_mode = (vane == VerticalVane::SWING || vane == VerticalVane::SWING_STEP)
                          ? (this->horizontal_vane_ == HorizontalVane::SWING ? climate::CLIMATE_SWING_BOTH
                                                                              : climate::CLIMATE_SWING_VERTICAL)
                          : (this->horizontal_vane_ == HorizontalVane::SWING ? climate::CLIMATE_SWING_HORIZONTAL
                                                                              : climate::CLIMATE_SWING_OFF);
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_horizontal_vane(HorizontalVane vane) {
  this->horizontal_vane_ = vane;
  this->vane_adjusted_ = true;
  this->swing_mode =
      (vane == HorizontalVane::SWING)
          ? (this->vertical_vane_ == VerticalVane::SWING || this->vertical_vane_ == VerticalVane::SWING_STEP
                 ? climate::CLIMATE_SWING_BOTH
                 : climate::CLIMATE_SWING_HORIZONTAL)
          : (this->vertical_vane_ == VerticalVane::SWING || this->vertical_vane_ == VerticalVane::SWING_STEP
                 ? climate::CLIMATE_SWING_VERTICAL
                 : climate::CLIMATE_SWING_OFF);
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_sensor_vane(SensorVane vane) {
  this->sensor_vane_ = vane;
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_circulation(bool enabled) {
  this->circulation_ = enabled;
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::set_streamer(bool enabled) {
  this->streamer_ = enabled;
  this->transmit_state();
  this->publish_state();
}

void DaikinArc472A66Climate::start_internal_clean() { this->send_special_command_(DAIKIN_SPECIAL_CMD_INTERNAL_CLEAN); }

void DaikinArc472A66Climate::start_filter_clean() { this->send_special_command_(DAIKIN_SPECIAL_CMD_FILTER_CLEAN); }

void DaikinArc472A66Climate::send_inquiry() {
  this->send_special_command_(DAIKIN_SPECIAL_CMD_INQUIRY, DAIKIN_SPECIAL_PAYLOAD_INQUIRY);
}

bool DaikinArc472A66Climate::parse_state_frame_(const uint8_t frame[]) {
  uint8_t checksum = 0;
  for (int i = 0; i < (DAIKIN_STATE_FRAME_SIZE - 1); i++) {
    checksum += frame[i];
  }
  if (frame[DAIKIN_STATE_FRAME_SIZE - 1] != checksum) {
    ESP_LOGI(TAG, "checksum error");
    return false;
  }

  char buf[DAIKIN_STATE_FRAME_SIZE * 3 + 1] = {0};
  size_t pos = 0;
  for (size_t i = 0; i < DAIKIN_STATE_FRAME_SIZE; i++) {
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%02x ", frame[i]);
  }
  ESP_LOGD(TAG, "FRAME %s", buf);

  uint8_t mode = frame[5];
  if (mode & DAIKIN_MODE_ON) {
    switch (mode & 0xF0) {
      case DAIKIN_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case DAIKIN_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case DAIKIN_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case DAIKIN_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case DAIKIN_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  uint8_t temperature = frame[6];
  if (!(temperature & 0xC0)) {
    this->target_temperature = temperature >> 1;
    this->target_temperature += (temperature & 0x1) ? 0.5 : 0;
  }
  this->target_humidity = frame[7];
  uint8_t fan_mode = frame[8];
  // 実測: swing_modeはstate frame側(byte8下位nibble/byte9)には現れない。
  // 風向はheader frame側の機能のため、ここでは触らない (2-5節参照)。
  switch (fan_mode & 0xF0) {
    case DAIKIN_FAN_1:
      this->set_custom_fan_mode_("1");
      break;
    case DAIKIN_FAN_2:
      this->set_custom_fan_mode_("2");
      break;
    case DAIKIN_FAN_3:
      this->set_custom_fan_mode_("3");
      break;
    case DAIKIN_FAN_4:
      this->set_custom_fan_mode_("4");
      break;
    case DAIKIN_FAN_5:
      this->set_custom_fan_mode_("5");
      break;
    case DAIKIN_FAN_SILENT:
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
      break;
    case DAIKIN_FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }

  uint8_t flags = frame[13];
  if (flags & DAIKIN_POWERFUL_BIT_MASK) {
    this->preset = climate::CLIMATE_PRESET_BOOST;
  } else if (flags & DAIKIN_SLEEP_BIT_MASK) {
    this->preset = climate::CLIMATE_PRESET_SLEEP;
  } else {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  this->comfort_mode_ = (frame[14] == DAIKIN_BIZ_AUTO_BYTE) ? ComfortBiasMode::BIZ : ComfortBiasMode::COMFORT;

  switch (frame[16]) {
    case static_cast<uint8_t>(SensorVane::AREA):
      this->sensor_vane_ = SensorVane::AREA;
      break;
    case static_cast<uint8_t>(SensorVane::SPOT):
      this->sensor_vane_ = SensorVane::SPOT;
      break;
    default:
      this->sensor_vane_ = SensorVane::OFF;
      break;
  }

  this->publish_state();
  return true;
}

bool DaikinArc472A66Climate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[DAIKIN_STATE_FRAME_SIZE] = {};

  bool valid_daikin_frame = false;
  if (data.expect_item(DAIKIN_HEADER_MARK, DAIKIN_HEADER_SPACE)) {
    valid_daikin_frame = true;
    size_t bytes_count = data.size() / 2 / 8;
    char buf[40 * 3 + 1] = {};
    size_t buf_pos = 0;
    for (size_t i = 0; i < bytes_count; i++) {
      uint8_t byte = 0;
      for (int8_t bit = 0; bit < 8; bit++) {
        if (data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ONE_SPACE)) {
          byte |= 1 << bit;
        } else if (!data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ZERO_SPACE)) {
          valid_daikin_frame = false;
          break;
        }
      }
      buf_pos += snprintf(buf + buf_pos, sizeof(buf) - buf_pos, "%02x ", byte);
    }
    ESP_LOGD(TAG, "WHOLE FRAME %s  size: %d", buf, data.size());
  }

  data.reset();

  if (!data.expect_item(DAIKIN_HEADER_MARK, DAIKIN_HEADER_SPACE)) {
    ESP_LOGI(TAG, "non daikin_arc472a66 expect item");
    return false;
  }

  for (uint8_t pos = 0; pos < DAIKIN_STATE_FRAME_SIZE; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ONE_SPACE)) {
        byte |= 1 << bit;
      } else if (!data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ZERO_SPACE)) {
        ESP_LOGI(TAG, "non daikin_arc472a66 expect item pos: %d", pos);
        return false;
      }
    }
    state_frame[pos] = byte;
    if (pos == 0 && byte != 0x11) {
      ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
      return false;
    } else if (pos == 1 && byte != 0xDA) {
      ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
      return false;
    } else if (pos == 2 && byte != 0x27) {
      ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
      return false;
    } else if (pos == 3 && byte != 0x00) {
      ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
      return false;
    } else if (pos == 4 && byte != 0x00) {
      ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
      return false;
    } else if (pos == 5) {
      if ((byte & 0x40) != 0x40) {
        ESP_LOGI(TAG, "non daikin_arc472a66 expect pos: %d header: %02x", pos, byte);
        return false;
      }
    }
  }
  return this->parse_state_frame_(state_frame);
}

void DaikinArc472A66Climate::control(const climate::ClimateCall &call) {
  auto target_humidity = call.get_target_humidity();
  if (target_humidity.has_value()) {
    this->target_humidity = *target_humidity;
  }

  if (call.has_custom_fan_mode()) {
    this->set_custom_fan_mode_(call.get_custom_fan_mode());
  } else if (call.get_fan_mode().has_value()) {
    // climate_ir::ClimateIR::control() only assigns this->fan_mode directly and
    // never clears a previously-set custom_fan_mode, so do it here to avoid
    // leaving both set at once when switching from a custom (1-5) speed back
    // to a standard one (AUTO/QUIET).
    this->clear_custom_fan_mode_();
  }

  auto swing_mode = call.get_swing_mode();
  if (swing_mode.has_value()) {
    switch (*swing_mode) {
      case climate::CLIMATE_SWING_OFF:
        this->vertical_vane_ = VerticalVane::AUTO;
        this->horizontal_vane_ = HorizontalVane::AUTO;
        break;
      case climate::CLIMATE_SWING_VERTICAL:
        this->vertical_vane_ = VerticalVane::SWING;
        break;
      case climate::CLIMATE_SWING_HORIZONTAL:
        this->horizontal_vane_ = HorizontalVane::SWING;
        break;
      case climate::CLIMATE_SWING_BOTH:
        this->vertical_vane_ = VerticalVane::SWING;
        this->horizontal_vane_ = HorizontalVane::SWING;
        break;
    }
    this->vane_adjusted_ = true;
  }

  climate_ir::ClimateIR::control(call);
}

}  // namespace daikin_arc472a66
}  // namespace esphome
