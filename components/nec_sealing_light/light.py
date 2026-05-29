import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, remote_transmitter
from esphome.const import CONF_ID

DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["remote_base"]

nec_sealing_light_ns = cg.esphome_ns.namespace("nec_sealing_light")
NecSealingLightComponent = nec_sealing_light_ns.class_(
    "NecSealingLightComponent", light.LightOutput
)

CONF_TRANSMITTER_ID = "transmitter_id"
CONF_LIGHTS = "lights"

LIGHT_ENTRY_SCHEMA = light.LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(NecSealingLightComponent),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
        cv.Required(CONF_LIGHTS): cv.ensure_list(LIGHT_ENTRY_SCHEMA),
    }
)


async def to_code(config):
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    for light_conf in config[CONF_LIGHTS]:
        var = cg.new_Pvariable(light_conf[CONF_ID])
        await light.register_light(var, light_conf)
        cg.add(var.set_transmitter(transmitter))
