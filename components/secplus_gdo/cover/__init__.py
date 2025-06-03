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
from esphome import automation
from esphome.components import cover
from esphome.const import CONF_TRIGGER_ID

from .. import SECPLUS_GDO_CONFIG_SCHEMA, secplus_gdo_ns, CONF_SECPLUS_GDO_ID

DEPENDENCIES = ["secplus_gdo"]

GDODoor = secplus_gdo_ns.class_("GDODoor", cover.Cover, cg.Component)

CoverClosingStartTrigger = secplus_gdo_ns.class_(
    "CoverClosingStartTrigger", automation.Trigger.template()
)

CoverClosingEndTrigger = secplus_gdo_ns.class_(
    "CoverClosingEndTrigger", automation.Trigger.template()
)

CONF_PRE_CLOSE_WARNING_DURATION = "pre_close_warning_duration"
CONF_PRE_CLOSE_WARNING_START = "pre_close_warning_start"
CONF_PRE_CLOSE_WARNING_END = "pre_close_warning_end"

CONFIG_SCHEMA = (
    cover.cover_schema(GDODoor)
    .extend(
        {
            cv.Optional(CONF_PRE_CLOSE_WARNING_DURATION, default=0): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_PRE_CLOSE_WARNING_START): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CoverClosingStartTrigger)}
            ),
            cv.Optional(CONF_PRE_CLOSE_WARNING_END): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CoverClosingEndTrigger)}
            ),
        }
    ).extend(SECPLUS_GDO_CONFIG_SCHEMA)
)

async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_SECPLUS_GDO_ID])
    cg.add(parent.register_door(var))
    cg.add(var.set_pre_close_warning_duration(config[CONF_PRE_CLOSE_WARNING_DURATION]))
    for conf in config.get(CONF_PRE_CLOSE_WARNING_START, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
        cg.add(var.register_door_closing_warn_start_trigger(trigger))
    for conf in config.get(CONF_PRE_CLOSE_WARNING_END, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
        cg.add(var.register_door_closing_warn_end_trigger(trigger))

