#pragma once

#include "esphome/core/automation.h"
#include "daikin_arc472a66.h"

namespace esphome {
namespace daikin_arc472a66 {

template<typename... Ts> class SetComfortModeAction final : public Action<Ts...> {
 public:
  SetComfortModeAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(ComfortBiasMode, mode)
  void play(const Ts &...x) override { this->parent_->set_comfort_mode(this->mode_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetComfortOffsetAction final : public Action<Ts...> {
 public:
  SetComfortOffsetAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(float, offset)
  void play(const Ts &...x) override { this->parent_->set_comfort_offset(this->offset_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetHumidifyAction final : public Action<Ts...> {
 public:
  SetHumidifyAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, enabled)
  void play(const Ts &...x) override { this->parent_->set_humidify(this->enabled_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetVerticalVaneAction final : public Action<Ts...> {
 public:
  SetVerticalVaneAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(VerticalVane, vane)
  void play(const Ts &...x) override { this->parent_->set_vertical_vane(this->vane_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetHorizontalVaneAction final : public Action<Ts...> {
 public:
  SetHorizontalVaneAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(HorizontalVane, vane)
  void play(const Ts &...x) override { this->parent_->set_horizontal_vane(this->vane_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetSensorVaneAction final : public Action<Ts...> {
 public:
  SetSensorVaneAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(SensorVane, mode)
  void play(const Ts &...x) override { this->parent_->set_sensor_vane(this->mode_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetCirculationAction final : public Action<Ts...> {
 public:
  SetCirculationAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, enabled)
  void play(const Ts &...x) override { this->parent_->set_circulation(this->enabled_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SetStreamerAction final : public Action<Ts...> {
 public:
  SetStreamerAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, enabled)
  void play(const Ts &...x) override { this->parent_->set_streamer(this->enabled_.value(x...)); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class StartInternalCleanAction final : public Action<Ts...> {
 public:
  StartInternalCleanAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->start_internal_clean(); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class StartFilterCleanAction final : public Action<Ts...> {
 public:
  StartFilterCleanAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->start_filter_clean(); }

 protected:
  DaikinArc472A66Climate *parent_;
};

template<typename... Ts> class SendInquiryAction final : public Action<Ts...> {
 public:
  SendInquiryAction(DaikinArc472A66Climate *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->send_inquiry(); }

 protected:
  DaikinArc472A66Climate *parent_;
};

}  // namespace daikin_arc472a66
}  // namespace esphome
