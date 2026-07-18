from esphome import automation
import esphome.codegen as cg
from esphome.components import climate_ir
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODE

AUTO_LOAD = ["climate_ir"]

daikin_arc472a66_ns = cg.esphome_ns.namespace("daikin_arc472a66")
DaikinArc472A66Climate = daikin_arc472a66_ns.class_(
    "DaikinArc472A66Climate", climate_ir.ClimateIR
)

ComfortBiasMode = daikin_arc472a66_ns.enum("ComfortBiasMode", True)
COMFORT_BIAS_MODE_OPTIONS = {
    "COMFORT": ComfortBiasMode.COMFORT,  # 快適エコ自動 (デフォルト)
    "BIZ": ComfortBiasMode.BIZ,  # ビズ自動
}

VerticalVane = daikin_arc472a66_ns.enum("VerticalVane", True)
VERTICAL_VANE_OPTIONS = {
    "POSITION_1": VerticalVane.POSITION_1,  # 一番上
    "POSITION_2": VerticalVane.POSITION_2,
    "POSITION_3": VerticalVane.POSITION_3,
    "POSITION_4": VerticalVane.POSITION_4,
    "POSITION_5": VerticalVane.POSITION_5,
    "POSITION_6": VerticalVane.POSITION_6,  # 一番下
    "SWING_STEP": VerticalVane.SWING_STEP,  # 揺らぎ
    "SWING": VerticalVane.SWING,  # スイング(首振り)
    "AUTO": VerticalVane.AUTO,  # 自動(センサー)
}

HorizontalVane = daikin_arc472a66_ns.enum("HorizontalVane", True)
HORIZONTAL_VANE_OPTIONS = {
    "WIDE": HorizontalVane.WIDE,  # 全体(左右ワイド固定)
    "POSITION_1": HorizontalVane.POSITION_1,  # 左端
    "POSITION_2": HorizontalVane.POSITION_2,
    "POSITION_3": HorizontalVane.POSITION_3,
    "POSITION_4": HorizontalVane.POSITION_4,  # 中央
    "POSITION_5": HorizontalVane.POSITION_5,
    "POSITION_6": HorizontalVane.POSITION_6,
    "POSITION_7": HorizontalVane.POSITION_7,  # 右端
    "SWING": HorizontalVane.SWING,
    "AUTO": HorizontalVane.AUTO,  # センサー自動
}

SensorVane = daikin_arc472a66_ns.enum("SensorVane", True)
SENSOR_VANE_OPTIONS = {
    "OFF": SensorVane.OFF,
    "AREA": SensorVane.AREA,  # エリア(広めの範囲を検知)
    "SPOT": SensorVane.SPOT,  # スポット(ピンポイント検知)
}

# 「停止ボタンを押したときに、内部クリーン専用の8バイト特殊フレームを
#  追加送信するか」を選べるオプション (PROTOCOL_NOTES.md 2-6節)。
# デフォルトfalse: 内部クリーンに入るかどうかは本来エアコン本体が自律的に
# 判断しており、この追加フレームを送ると「自律的に開始した内部クリーンを
# キャンセルするトグル」と誤解釈され、正常な停止アナウンスも内部クリーン
# 開始も行われなくなる不具合が実機で確認されたため。
CONF_POWER_OFF_INTERNAL_CLEAN = "power_off_internal_clean"
CONF_OFFSET = "offset"
CONF_ENABLED = "enabled"
CONF_VANE = "vane"

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(
    DaikinArc472A66Climate
).extend(
    {
        cv.Optional(CONF_POWER_OFF_INTERNAL_CLEAN, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_power_off_internal_clean(config[CONF_POWER_OFF_INTERNAL_CLEAN]))


# ============================================================
# オートメーションアクション
#
# climateの標準機能(mode/fan_mode/custom_fan_mode/target_temperature/
# swing_mode/preset)には収まらない、ARC472A66固有の機能
# (PROTOCOL_NOTES.md参照)をここで automation action として公開する。
# ============================================================

SetComfortModeAction = daikin_arc472a66_ns.class_(
    "SetComfortModeAction", automation.Action
)
SetComfortOffsetAction = daikin_arc472a66_ns.class_(
    "SetComfortOffsetAction", automation.Action
)
SetHumidifyAction = daikin_arc472a66_ns.class_("SetHumidifyAction", automation.Action)
SetVerticalVaneAction = daikin_arc472a66_ns.class_(
    "SetVerticalVaneAction", automation.Action
)
SetHorizontalVaneAction = daikin_arc472a66_ns.class_(
    "SetHorizontalVaneAction", automation.Action
)
SetSensorVaneAction = daikin_arc472a66_ns.class_(
    "SetSensorVaneAction", automation.Action
)
SetCirculationAction = daikin_arc472a66_ns.class_(
    "SetCirculationAction", automation.Action
)
SetStreamerAction = daikin_arc472a66_ns.class_("SetStreamerAction", automation.Action)
StartInternalCleanAction = daikin_arc472a66_ns.class_(
    "StartInternalCleanAction", automation.Action
)
StartFilterCleanAction = daikin_arc472a66_ns.class_(
    "StartFilterCleanAction", automation.Action
)
SendInquiryAction = daikin_arc472a66_ns.class_("SendInquiryAction", automation.Action)

DAIKIN_ARC472A66_SIMPLE_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
    }
)


@automation.register_action(
    "climate.daikin_arc472a66.set_comfort_mode",
    SetComfortModeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_MODE): cv.templatable(
                cv.enum(COMFORT_BIAS_MODE_OPTIONS, upper=True)
            ),
        }
    ),
    synchronous=True,
)
async def set_comfort_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_MODE], args, ComfortBiasMode)
    cg.add(var.set_mode(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_comfort_offset",
    SetComfortOffsetAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_OFFSET): cv.templatable(
                cv.float_range(min=-1.0, max=2.0)
            ),
        }
    ),
    synchronous=True,
)
async def set_comfort_offset_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_OFFSET], args, cg.float_)
    cg.add(var.set_offset(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_humidify",
    SetHumidifyAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_ENABLED): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def set_humidify_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_ENABLED], args, cg.bool_)
    cg.add(var.set_enabled(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_vertical_vane",
    SetVerticalVaneAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_VANE): cv.templatable(
                cv.enum(VERTICAL_VANE_OPTIONS, upper=True)
            ),
        }
    ),
    synchronous=True,
)
async def set_vertical_vane_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_VANE], args, VerticalVane)
    cg.add(var.set_vane(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_horizontal_vane",
    SetHorizontalVaneAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_VANE): cv.templatable(
                cv.enum(HORIZONTAL_VANE_OPTIONS, upper=True)
            ),
        }
    ),
    synchronous=True,
)
async def set_horizontal_vane_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_VANE], args, HorizontalVane)
    cg.add(var.set_vane(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_sensor_vane",
    SetSensorVaneAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_MODE): cv.templatable(
                cv.enum(SENSOR_VANE_OPTIONS, upper=True)
            ),
        }
    ),
    synchronous=True,
)
async def set_sensor_vane_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_MODE], args, SensorVane)
    cg.add(var.set_mode(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_circulation",
    SetCirculationAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_ENABLED): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def set_circulation_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_ENABLED], args, cg.bool_)
    cg.add(var.set_enabled(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.set_streamer",
    SetStreamerAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DaikinArc472A66Climate),
            cv.Required(CONF_ENABLED): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def set_streamer_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_ENABLED], args, cg.bool_)
    cg.add(var.set_enabled(template_))
    return var


@automation.register_action(
    "climate.daikin_arc472a66.start_internal_clean",
    StartInternalCleanAction,
    DAIKIN_ARC472A66_SIMPLE_ACTION_SCHEMA,
    synchronous=True,
)
@automation.register_action(
    "climate.daikin_arc472a66.start_filter_clean",
    StartFilterCleanAction,
    DAIKIN_ARC472A66_SIMPLE_ACTION_SCHEMA,
    synchronous=True,
)
@automation.register_action(
    "climate.daikin_arc472a66.send_inquiry",
    SendInquiryAction,
    DAIKIN_ARC472A66_SIMPLE_ACTION_SCHEMA,
    synchronous=True,
)
async def daikin_arc472a66_simple_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
