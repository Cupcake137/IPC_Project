#include "ipc_can_db.h"

static uint8_t update_crc(uint8_t crc, uint8_t value)
{
    crc ^= value;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((crc & 0x80) != 0) {
            crc = (uint8_t)((crc << 1) ^ 0x1D);
        } else {
            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static bool begin_frame(CAN_Frame_t* frame, uint16_t id, uint8_t dlc)
{
    if (frame == nullptr || dlc == 0 || dlc > CAN_MAX_DLC) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->id = id;
    frame->dlc = dlc;
    return true;
}

static void finish_frame(CAN_Frame_t* frame)
{
    frame->data[frame->dlc - 1] = can_calculate_crc(*frame);
}

uint8_t can_calculate_crc(const CAN_Frame_t& frame)
{
    if (frame.dlc == 0 || frame.dlc > CAN_MAX_DLC) {
        return 0;
    }

    uint8_t crc = 0xFF;
    crc = update_crc(crc, (uint8_t)(frame.id >> 8));
    crc = update_crc(crc, (uint8_t)(frame.id & 0xFF));
    crc = update_crc(crc, frame.dlc);
    for (uint8_t index = 0; index + 1 < frame.dlc; ++index) {
        crc = update_crc(crc, frame.data[index]);
    }
    return (uint8_t)(crc ^ 0xFF);
}

bool can_validate(const CAN_Frame_t& frame, uint16_t id, uint8_t dlc)
{
    return dlc > 0
        && dlc <= CAN_MAX_DLC
        && frame.id == id
        && frame.dlc == dlc
        && frame.data[dlc - 1] == can_calculate_crc(frame);
}

bool can_pack_motor_command(const MotorCommand& command, CAN_Frame_t* frame)
{
    if (command.gear > 3 || !begin_frame(frame, CAN_VCU_MOTOR_COMMAND, 4)) {
        return false;
    }
    frame->data[0] = command.pwm;
    frame->data[1] = (uint8_t)(command.gear
        | (command.torqueEnable ? 0x04 : 0x00));
    frame->data[2] = command.aliveCounter & 0x0F;
    finish_frame(frame);
    return true;
}

bool can_unpack_motor_command(const CAN_Frame_t& frame, MotorCommand* command)
{
    if (command == nullptr || !can_validate(frame, CAN_VCU_MOTOR_COMMAND, 4)) {
        return false;
    }
    command->pwm = frame.data[0];
    command->gear = frame.data[1] & 0x03;
    command->torqueEnable = (frame.data[1] & 0x04) != 0;
    command->aliveCounter = frame.data[2] & 0x0F;
    return command->gear <= 3;
}

bool can_pack_drive_status(uint8_t pedal, uint8_t speed, uint8_t gear,
                           uint8_t appliedPwm, uint8_t counter,
                           CAN_Frame_t* frame)
{
    if (pedal > 100 || gear > 3
        || !begin_frame(frame, CAN_ECU_DRIVE_STATUS, 6)) {
        return false;
    }
    frame->data[0] = pedal;
    frame->data[1] = speed;
    frame->data[2] = gear;
    frame->data[3] = appliedPwm;
    frame->data[4] = counter & 0x0F;
    finish_frame(frame);
    return true;
}

bool can_pack_energy_status(uint8_t soc, uint8_t counter, CAN_Frame_t* frame)
{
    if (soc > 100 || !begin_frame(frame, CAN_ECU_ENERGY_STATUS, 3)) {
        return false;
    }
    frame->data[0] = soc;
    frame->data[1] = counter & 0x0F;
    finish_frame(frame);
    return true;
}

bool can_pack_diag_status(uint8_t dtc, uint8_t flags, uint8_t counter,
                          CAN_Frame_t* frame)
{
    if (!begin_frame(frame, CAN_ECU_DIAG_STATUS, 4)) {
        return false;
    }
    frame->data[0] = dtc;
    frame->data[1] = flags & 0x0F;
    frame->data[2] = counter & 0x0F;
    finish_frame(frame);
    return true;
}

void can_print_frame(Stream& output, const CAN_Frame_t& frame)
{
    if (frame.id < 0x100) {
        output.print('0');
    }
    if (frame.id < 0x010) {
        output.print('0');
    }
    output.print(frame.id, HEX);
    output.print('#');
    for (uint8_t index = 0; index < frame.dlc; ++index) {
        if (frame.data[index] < 0x10) {
            output.print('0');
        }
        output.print(frame.data[index], HEX);
    }
    output.println();
}
