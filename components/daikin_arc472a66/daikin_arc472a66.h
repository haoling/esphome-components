#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace daikin_arc472a66 {

// ============================================================
// ARC472A66 (Daikin) 赤外線リモコン再現コンポーネント。
//
// 本家 esphome/components/daikin_arc をベースに、実機リモコン(ARC472A66)を
// M5Stack Atom Lite + Unit IR で解析した結果を反映したもの。
// 詳細な解析メモは PROTOCOL_NOTES.md を参照。
//
// 実機解析で判明した本家との相違点:
// - state frame(19バイト)の byte[17] は本家の 0x24 ではなく 0x2a。
// - 停止(電源OFF)時、byte[5](モード)・byte[6](温度)は "OFF専用の固定値" に
//   潰されず、直前に使っていたモードのビットを残したまま電源ビットだけ落とす
//   (last_active_mode_ で実装)。
// - 風向上下/左右・サーキュレーション・Streamer(空清)は19バイトのstate frame
//   には一切現れず、20バイトのheader frame側の別バイトで表現される。既存の
//   climate_ir 標準実装(state frameのswing_mode操作)はこのリモコンでは
//   一切効かないため、header frameを動的に組み立てるよう作り直してある。
// - 内部クリーン・フィルター掃除・お知らせは、19/20バイトのフレームとは別の
//   8バイト特殊フレーム (`11 DA 27 00 84 <sub> <payload> <checksum>`) として
//   単発送信される。
// ============================================================

// Temperature (絶対温度モード: 冷房・暖房)
const uint8_t DAIKIN_TEMP_MIN = 10;  // Celsius (実測範囲外。境界値は未検証)
const uint8_t DAIKIN_TEMP_MAX = 30;  // Celsius (実測範囲外。境界値は未検証)

// 除湿モードの湿度設定 (byte[7], 生パーセント値をそのまま送る。オフセットなし)
// 実測は 55% / 60% の2点のみ。上下限は一般的なダイキン除湿機の可変範囲から仮定。
const uint8_t DAIKIN_HUMIDITY_MIN = 40;
const uint8_t DAIKIN_HUMIDITY_MAX = 60;

// 快適エコ自動(HEAT_COOL)モードの温度オフセット可変範囲 (実測確認済みの範囲のみ)
const float DAIKIN_COMFORT_OFFSET_MIN = -1.0f;
const float DAIKIN_COMFORT_OFFSET_MAX = 2.0f;

// Modes (byte[5] の下位4bit。実測で本家と完全一致することを確認済み)
const uint8_t DAIKIN_MODE_AUTO = 0x00;  // 快適エコ自動 / ビズ自動
const uint8_t DAIKIN_MODE_COOL = 0x30;
const uint8_t DAIKIN_MODE_HEAT = 0x40;
const uint8_t DAIKIN_MODE_DRY = 0x20;
const uint8_t DAIKIN_MODE_FAN = 0x60;
const uint8_t DAIKIN_MODE_OFF = 0x00;
const uint8_t DAIKIN_MODE_ON = 0x01;

// Fan Speed (byte[8]上位nibble。実測で本家と完全一致することを確認済み)
const uint8_t DAIKIN_FAN_AUTO = 0xA0;
const uint8_t DAIKIN_FAN_SILENT = 0xB0;
const uint8_t DAIKIN_FAN_1 = 0x30;
const uint8_t DAIKIN_FAN_2 = 0x40;
const uint8_t DAIKIN_FAN_3 = 0x50;
const uint8_t DAIKIN_FAN_4 = 0x60;
const uint8_t DAIKIN_FAN_5 = 0x70;

// state frame byte[13]: パワフル・おやすみは独立したビットフラグ (実測確認済み)
const uint8_t DAIKIN_POWERFUL_BIT_MASK = 0x01;
const uint8_t DAIKIN_SLEEP_BIT_MASK = 0x04;

// state frame byte[14]: 快適エコ自動時のみ、快適エコ⇔ビズ自動をトグルするビット
const uint8_t DAIKIN_COMFORT_AUTO_BYTE = 0x0a;
const uint8_t DAIKIN_BIZ_AUTO_BYTE = 0x0e;

// IR Transmission タイミング (実測値と一致。変更不要)
const uint32_t DAIKIN_IR_FREQUENCY = 38000;
const uint32_t DAIKIN_ARC_PRE_MARK = 9950;
const uint32_t DAIKIN_ARC_PRE_SPACE = 25100;
const uint32_t DAIKIN_HEADER_MARK = 3450;
const uint32_t DAIKIN_HEADER_SPACE = 1760;
const uint32_t DAIKIN_BIT_MARK = 400;
const uint32_t DAIKIN_ONE_SPACE = 1300;
const uint32_t DAIKIN_ZERO_SPACE = 480;
const uint32_t DAIKIN_MESSAGE_SPACE = 35000;

const uint8_t DAIKIN_DBG_TOLERANCE = 25;
#define DAIKIN_DBG_LOWER(x) ((100 - DAIKIN_DBG_TOLERANCE) * (x) / 100U)
#define DAIKIN_DBG_UPPER(x) ((100 + DAIKIN_DBG_TOLERANCE) * (x) / 100U)

// Frame sizes
const uint8_t DAIKIN_HEADER_FRAME_SIZE = 20;
const uint8_t DAIKIN_STATE_FRAME_SIZE = 19;
const uint8_t DAIKIN_SPECIAL_FRAME_SIZE = 8;

// 内部クリーン・フィルター掃除・お知らせ用、8バイト特殊フレームのサブコマンド
// (`11 DA 27 00 84 <sub> <payload> <checksum>`)
const uint8_t DAIKIN_SPECIAL_CMD_INTERNAL_CLEAN = 0x0c;
const uint8_t DAIKIN_SPECIAL_CMD_FILTER_CLEAN = 0x14;
const uint8_t DAIKIN_SPECIAL_CMD_INQUIRY = 0x87;
const uint8_t DAIKIN_SPECIAL_PAYLOAD_INQUIRY = 0x20;

// 自動復帰設定 (header frame byte[11])。
// リモコンのメニュー内から設定する「電源ON操作から一定時間で自動的に運転を
// 止める」機能(オフ/30分/1時間の3段階)。実機で以下の3値を確認済み:
// オフ=0x30 / 30分=0x20 / 1時間=0x10。冷房・暖房どちらのモードでも同じ値
// (モード依存ではない)。デフォルトはオフ(このビットが常に0x30以外の値に
// なると自動復帰が有効になってしまうため)。
enum class AutoReturn : uint8_t {
  OFF = 0x30,
  THIRTY_MINUTES = 0x20,
  ONE_HOUR = 0x10,
};

// 風向上下 (header frame byte[12])。
// 【訂正】以前は byte[11] と記録されていたが、実機の複数キャプチャ
// (暖房23℃時に POSITION_3=0x36 が観測された位置、自動復帰検証時に
// POSITION_1=0x16/AUTO=0xE6 が観測された位置)から、正しくは byte[12] で
// あることが判明した。byte[11] は自動復帰設定用のバイト(上記参照)であり、
// 以前の実装がここに風向上下の値を書き込んでいたため、常に自動復帰が
// 意図しない値で送信される不具合の原因になっていた。
enum class VerticalVane : uint8_t {
  POSITION_1 = 0x16,  // 一番上
  POSITION_2 = 0x26,
  POSITION_3 = 0x36,
  POSITION_4 = 0x46,
  POSITION_5 = 0x56,
  POSITION_6 = 0x66,  // 一番下
  SWING_STEP = 0xC6,  // 揺らぎ (小刻みに揺れる特殊パターン)
  SWING = 0xF6,       // 通常のスイング(首振り)
  AUTO = 0xE6,        // 自動(センサー)
};

// 風向左右 (header frame byte[13]、実測確認済み)
enum class HorizontalVane : uint8_t {
  WIDE = 0x21,  // 全体(左右ワイド固定)
  POSITION_1 = 0x28,  // 左端
  POSITION_2 = 0x29,
  POSITION_3 = 0x2a,
  POSITION_4 = 0x2b,  // 中央
  POSITION_5 = 0x2c,
  POSITION_6 = 0x2d,
  POSITION_7 = 0x2e,  // 右端
  SWING = 0x3f,       // スイング
  AUTO = 0x3e,        // センサー自動
};

// センサー風向ボタン (state frame byte[16] の3値サイクル、実測確認済み)
// ONのときはheader側の上下・左右を自動値(AUTO)に強制する。
enum class SensorVane : uint8_t {
  OFF = 0x80,    // 通常の固定/自動角度に戻る
  AREA = 0x83,   // エリア(広めの範囲を検知)
  SPOT = 0x84,   // スポット(ピンポイント検知)
};

// 快適エコ自動モード中の「快適エコ自動」⇔「ビズ自動」トグル (state frame byte[14])
enum class ComfortBiasMode : uint8_t {
  COMFORT = DAIKIN_COMFORT_AUTO_BYTE,
  BIZ = DAIKIN_BIZ_AUTO_BYTE,
};

class DaikinArc472A66Climate final : public climate_ir::ClimateIR {
 public:
  DaikinArc472A66Climate()
      : climate_ir::ClimateIR(DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX, 0.5f, true, true,
                               {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_QUIET},
                               {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                                climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH},
                               {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_BOOST,
                                climate::CLIMATE_PRESET_SLEEP}) {}

  void setup() override;

  // ---- YAML options ----
  void set_power_off_internal_clean(bool value) { this->power_off_internal_clean_ = value; }
  void set_auto_return(AutoReturn value) { this->auto_return_ = value; }

  // ---- オートメーションのアクションから呼ばれるsetter群 ----
  // (climateの標準機能(mode/fan_mode/target_temperature/swing_mode/preset)には
  //  収まらない、このリモコン固有の機能を automation action として公開する)
  void set_comfort_mode(ComfortBiasMode mode);
  void set_comfort_offset(float offset_celsius);
  void set_humidify(bool enabled);
  void set_vertical_vane(VerticalVane vane);
  void set_horizontal_vane(HorizontalVane vane);
  void set_sensor_vane(SensorVane vane);
  void set_circulation(bool enabled);
  void set_streamer(bool enabled);
  void start_internal_clean();
  void start_filter_clean();
  void send_inquiry();

 protected:
  void control(const climate::ClimateCall &call) override;
  void transmit_state() override;
  climate::ClimateTraits traits() override;

  uint8_t operation_mode_();
  uint16_t fan_speed_();
  uint8_t temperature_();
  uint8_t humidity_();
  uint8_t vertical_vane_byte_();
  uint8_t horizontal_vane_byte_();
  void send_special_command_(uint8_t sub_code, uint8_t payload = 0x00);

  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);

  // climate::ClimateMode → ダイキンのモードビット (電源ビットは含まない)
  static uint8_t mode_bits_for_(climate::ClimateMode mode);

  uint8_t last_humidity_{0x66};
  // デフォルトfalse: 内部クリーンへの移行はエアコン本体が自律的に判断するため
  // (daikin_arc472a66.cpp の transmit_state() 内コメント・PROTOCOL_NOTES.md 2-6節参照)
  bool power_off_internal_clean_{false};

  // 自動復帰設定 (header byte[11])。デフォルトはオフ。
  AutoReturn auto_return_{AutoReturn::OFF};

  // 停止(OFF)フレーム送信時に参照する「直前に使っていたモード」。
  // 実測により、停止時もこのモードのビット/温度エンコードが維持されるため。
  climate::ClimateMode last_active_mode_{climate::CLIMATE_MODE_COOL};

  // 快適エコ自動(HEAT_COOL)モード関連
  ComfortBiasMode comfort_mode_{ComfortBiasMode::COMFORT};
  float comfort_offset_{0.0f};

  // 暖房モードの加湿 (byte[7] = 0xff)
  bool humidify_{false};

  // 風向・サーキュレーション・Streamer (いずれも header frame側の機能)
  VerticalVane vertical_vane_{VerticalVane::AUTO};
  HorizontalVane horizontal_vane_{HorizontalVane::AUTO};
  SensorVane sensor_vane_{SensorVane::OFF};
  bool circulation_{false};
  bool streamer_{true};
  // 風向上下/左右ボタンが一度でも操作されたか (header byte[9] の実測挙動再現用)
  bool vane_adjusted_{false};
};

}  // namespace daikin_arc472a66
}  // namespace esphome
