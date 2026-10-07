#ifndef IPC_CAN_DB_H
#define IPC_CAN_DB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define IPC_CAN_STANDARD_ID_MAX 0x7FFU
#define IPC_CAN_MAX_DLC 8U

typedef struct can_frame {
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t data[IPC_CAN_MAX_DLC];
} CAN_Frame_t;

typedef enum {
    CAN_ID_VCU_MOTOR_COMMAND = 0x080,
    CAN_ID_ECU_DRIVE_STATUS = 0x100,
    CAN_ID_ECU_ENERGY_STATUS = 0x101,
    CAN_ID_ECU_DIAG_STATUS = 0x102,
    CAN_ID_SWC_KEY_EVENT = 0x300,
    CAN_ID_SWC_NETWORK_STATUS = 0x301,
} IpcCanId_t;

typedef enum {
    IPC_GEAR_P = 0,
    IPC_GEAR_R = 1,
    IPC_GEAR_N = 2,
    IPC_GEAR_D = 3,
} IpcGear_t;

enum {
    ECU_DIAG_FAULT_PRESENT = 1U << 0,
    ECU_DIAG_LOW_SOC = 1U << 1,
    ECU_DIAG_MOTOR_OVERLOAD = 1U << 2,
    ECU_DIAG_COMMAND_TIMEOUT = 1U << 3,
};

typedef struct {
    uint8_t pedal_percent;
    uint8_t speed_kmh;
    uint8_t gear;
    uint8_t applied_pwm;
    uint8_t alive_counter;
} EcuDriveStatus_t;

typedef struct {
    uint8_t battery_soc;
    uint8_t alive_counter;
} EcuEnergyStatus_t;

typedef struct {
    uint8_t active_dtc;
    uint8_t fault_flags;
    uint8_t alive_counter;
} EcuDiagStatus_t;

typedef struct {
    uint8_t authorized_pwm;
    uint8_t expected_gear;
    bool torque_enable;
    uint8_t alive_counter;
} VcuMotorCommand_t;

uint8_t ipc_can_crc8(const CAN_Frame_t* frame);
bool ipc_can_finalize(CAN_Frame_t* frame);
bool ipc_can_validate(const CAN_Frame_t* frame, uint16_t expected_id, uint8_t expected_dlc);
void ipc_can_trace(FILE* stream, const char* channel, const CAN_Frame_t* frame);

bool ipc_can_pack_ecu_drive_status(CAN_Frame_t* frame, const EcuDriveStatus_t* status);
bool ipc_can_unpack_ecu_drive_status(const CAN_Frame_t* frame, EcuDriveStatus_t* status);
bool ipc_can_pack_ecu_energy_status(CAN_Frame_t* frame, const EcuEnergyStatus_t* status);
bool ipc_can_unpack_ecu_energy_status(const CAN_Frame_t* frame, EcuEnergyStatus_t* status);
bool ipc_can_pack_ecu_diag_status(CAN_Frame_t* frame, const EcuDiagStatus_t* status);
bool ipc_can_unpack_ecu_diag_status(const CAN_Frame_t* frame, EcuDiagStatus_t* status);
bool ipc_can_pack_vcu_motor_command(CAN_Frame_t* frame, const VcuMotorCommand_t* command);
bool ipc_can_unpack_vcu_motor_command(const CAN_Frame_t* frame, VcuMotorCommand_t* command);
#endif
