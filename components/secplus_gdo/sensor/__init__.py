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
from esphome.components import sensor
from esphome.const import CONF_ID

from .. import SECPLUS_GDO_CONFIG_SCHEMA, secplus_gdo_ns, CONF_SECPLUS_GDO_ID

DEPENDENCIES = ["secplus_gdo"]

GDOStat = secplus_gdo_ns.class_("GDOStat", sensor.Sensor, cg.Component)

CONF_TYPE = "type"
TYPES = {
    "openings": "register_openings",
    "paired_devices_total": "register_paired_total",
    "paired_devices_remotes": "register_paired_remotes",
    "paired_devices_keypads": "register_paired_keypads",
    "paired_devices_wall_controls": "register_paired_wall_controls",
    "paired_devices_accessories":"register_paired_accessories",
}


CONFIG_SCHEMA = (
    sensor.sensor_schema(GDOStat)
    .extend(
        {
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(SECPLUS_GDO_CONFIG_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_SECPLUS_GDO_ID])
    fcall = str(parent) + "->" + str(TYPES[config[CONF_TYPE]])
    text = fcall + "(std::bind(&" + str(GDOStat) + "::publish_state," + str(config[CONF_ID]) + ",std::placeholders::_1))"
    cg.add((cg.RawExpression(text)))