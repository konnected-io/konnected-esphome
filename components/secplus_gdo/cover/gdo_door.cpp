#include "gdo_door.h"

namespace esphome {
namespace secplus_gdo {

void GDODoor::set_state(gdo_door_state_t state, float position) {
    if (this->pre_close_active_) {
        // If we are in the pre-close state and the door is closing,
        // then it was triggered by something else and we need to cancel the pre-close
        if (state == GDO_DOOR_STATE_CLOSING) {
            this->cancel_timeout("pre_close");
            this->pre_close_active_ = false;
            if (this->pre_close_end_trigger) {
                this->pre_close_end_trigger->trigger();
            }
        } else {
            // If we are in the pre-close state, and the door is not closing, then do not update
            // the state so that the stop button remains active on the front end to cancel the closing operation
            return;
        }
    }

    switch (state) {
    case GDO_DOOR_STATE_OPEN:
        this->position = COVER_OPEN;
        this->current_operation = COVER_OPERATION_IDLE;
        break;
    case GDO_DOOR_STATE_CLOSED:
        this->position = COVER_CLOSED;
        this->current_operation = COVER_OPERATION_IDLE;
        break;
    case GDO_DOOR_STATE_OPENING:
        this->current_operation = COVER_OPERATION_OPENING;
        this->position = position;
        break;
    case GDO_DOOR_STATE_CLOSING:
        this->current_operation = COVER_OPERATION_CLOSING;
        this->position = position;
        break;
    case GDO_DOOR_STATE_STOPPED:
    case GDO_DOOR_STATE_MAX:
    default:
        this->current_operation = COVER_OPERATION_IDLE;
        this->position = position;
        break;
    }

    this->publish_state(false);
}

void GDODoor::start_pre_close(uint32_t pos) {
    if (this->pre_close_active_) {
        return;
    }

    if (this->pre_close_start_trigger) {
        this->pre_close_start_trigger->trigger();
    }

    this->set_state(GDO_DOOR_STATE_CLOSING, this->position);

    this->set_timeout("pre_close", this->pre_close_duration_, [=]() {
        this->pre_close_active_ = false;
        if (this->pre_close_end_trigger) {
            this->pre_close_end_trigger->trigger();
        }
        if (pos > 0) {
            gdo_door_move_to_target(pos);
        } else {
            gdo_door_close();
        }
    });

    this->pre_close_active_ = true;
}

void GDODoor::control(const cover::CoverCall& call) {
    if (call.get_stop()) {
        if (this->pre_close_active_) {
            this->cancel_timeout("pre_close");
            this->pre_close_active_ = false;
            if (this->pre_close_end_trigger) {
                this->pre_close_end_trigger->trigger();
            }
            this->set_state(GDO_DOOR_STATE_OPEN, this->position);
        }

        gdo_door_stop();
    }

    if (call.get_toggle()) {
        if (this->position != COVER_CLOSED) {
            this->start_pre_close();
        } else {
            gdo_door_toggle();
        }
    }

    if (call.get_position().has_value()) {
        auto pos = *call.get_position();

        if (this->pre_close_active_) {
            this->cancel_timeout("pre_close");
            this->pre_close_active_ = false;
            if (this->pre_close_end_trigger) {
                this->pre_close_end_trigger->trigger();
            }
        }

        if (this->current_operation == COVER_OPERATION_OPENING ||
            this->current_operation == COVER_OPERATION_CLOSING) {
            gdo_door_stop();
        }

        if (pos == COVER_OPEN) {
            gdo_door_open();
        } else if (pos == COVER_CLOSED) {
            this->start_pre_close();
        } else {
            if (pos < this->position) {
                this->start_pre_close(10000 - (pos * 10000));
            } else {
                gdo_door_move_to_target(10000 - (pos * 10000));
            }
        }
    }
}

} // namespace secplus_gdo
} // namespace esphome