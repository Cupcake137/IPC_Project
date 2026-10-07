#ifndef IPC_CAN_DB_H
#define IPC_CAN_DB_H

#include <Arduino.h>

static const uint8_t CAN_MAX_DLC = 8;

enum CanId : uint16_t {
    CAN_VCU_MOTOR_COMMAND = 0x080,
    CAN_ECU_DRIVE_STATUS = 0x100,
    CAN_ECU_ENERGY_STATUS = 0x101,
    CAN_ECU_DIAG_STATUS = 0x102
};

enum DiagnosticFlag : uint8_t {
    DIAG_FAULT_PRESENT = 0x01,
    DIAG_LOW_SOC = 0x02,
    DIAG_MOTOR_OVERLOAD = 0x04,
    DIAG_COMMAND_TIMEOUT = 0x08
};

struct CAN_Frame_t {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[CAN_MAX_DLC];
};

struct MotorCommand {
    uint8_t pwm;
    uint8_t gear;
    bool torqueEnable;
    uint8_t aliveCounter;
};

uint8_t can_calculate_crc(const CAN_Frame_t& frame);
bool can_validate(const CAN_Frame_t& frame, uint16_t id, uint8_t dlc);
bool can_pack_motor_command(const MotorCommand& command, CAN_Frame_t* frame);
bool can_unpack_motor_command(const CAN_Frame_t& frame, MotorCommand* command);
bool can_pack_drive_status(uint8_t pedal, uint8_t speed, uint8_t gear,
                           uint8_t appliedPwm, uint8_t counter,
                           CAN_Frame_t* frame);
bool can_pack_energy_status(uint8_t soc, uint8_t counter, CAN_Frame_t* frame);
bool can_pack_diag_status(uint8_t dtc, uint8_t flags, uint8_t counter,
                          CAN_Frame_t* frame);
void can_print_frame(Stream& output, const CAN_Frame_t& frame);

#endif
