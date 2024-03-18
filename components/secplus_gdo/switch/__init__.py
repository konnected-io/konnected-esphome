import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import SECPLUS_GDO_CONFIG_SCHEMA, secplus_gdo_ns, CONF_SECPLUS_GDO_ID

DEPENDENCIES = ["secplus_gdo"]

GDOSwitch = secplus_gdo_ns.class_("GDOSwitch", switch.Switch, cg.Component)

CONF_TYPE = "type"
TYPES = {
    "learn": "register_learn",
}


CONFIG_SCHEMA = (
    switch.switch_schema(GDOSwitch)
    .extend(
        {
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(SECPLUS_GDO_CONFIG_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await switch.register_switch(var, config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_SECPLUS_GDO_ID])
    fcall = str(parent) + "->" + str(TYPES[config[CONF_TYPE]])
    text = fcall + "(std::bind(&" + str(GDOSwitch) + "::write_state," + str(config[CONF_ID]) + ",std::placeholders::_1))"
    cg.add((cg.RawExpression(text)))