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
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID  # New in 2023.5

from .. import SECPLUS_GDO_CONFIG_SCHEMA, secplus_gdo_ns, CONF_SECPLUS_GDO_ID

DEPENDENCIES = ["secplus_gdo"]

GDOLight = secplus_gdo_ns.class_(
    "GDOLight", light.LightOutput, cg.Component
)


CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend(
    {cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(GDOLight)}
).extend(SECPLUS_GDO_CONFIG_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    parent = await cg.get_variable(config[CONF_SECPLUS_GDO_ID])
    text = "std::bind(&" + str(GDOLight) + "::set_state," + str(config[CONF_OUTPUT_ID]) + ",std::placeholders::_1)"
    cg.add(parent.register_light(cg.RawExpression(text)))