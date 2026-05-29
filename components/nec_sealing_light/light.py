import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, remote_transmitter
from esphome.const import CONF_OUTPUT_ID

DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["remote_base"]

nec_sealing_light_ns = cg.esphome_ns.namespace("nec_sealing_light")
NecSealingLightComponent = nec_sealing_light_ns.class_(
    "NecSealingLightComponent", light.LightOutput
)

CONF_TRANSMITTER_ID = "transmitter_id"

CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(NecSealingLightComponent),
        cv.Required(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
