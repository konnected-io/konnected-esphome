"""
/*
 * Copyright (C) 2024  Konnected Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 """

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from .. import SECPLUS_GDO_CONFIG_SCHEMA, secplus_gdo_ns, CONF_SECPLUS_GDO_ID

DEPENDENCIES = ["secplus_gdo"]

GDONumber = secplus_gdo_ns.class_("GDONumber", number.Number, cg.Component)

CONF_TYPE = "type"
TYPES = {
    "open_duration": "register_open_duration",
    "close_duration": "register_close_duration",
}

CONFIG_SCHEMA = (
    number.number_schema(GDONumber)
    .extend(
        {
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(SECPLUS_GDO_CONFIG_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await number.register_number(var, config, min_value=0, max_value=65535, step=1)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_SECPLUS_GDO_ID])
    fcall = str(parent) + "->" + str(TYPES[config[CONF_TYPE]])
    text = fcall + "(" + str(var) + ")"
    cg.add((cg.RawExpression(text)))
    text = "gdo_set_" + str(config[CONF_TYPE])
    cg.add(var.set_control_function(cg.RawExpression(text)))
