#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "can_manager.h"
#include "ipc_can_db.h"
#include "vcu_core.h"

#define DEFAULT_SERIAL_PORT "/dev/ttyUSB0"
#define SERIAL_BAUD B9600

static int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        return -1;
    }
    cfmakeraw(&tty);
    cfsetispeed(&tty, SERIAL_BAUD);
    cfsetospeed(&tty, SERIAL_BAUD);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    return tcsetattr(fd, TCSANOW, &tty);
}

static long monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

static bool send_motor_command(int serial_port, VCU_VehicleContext_t* ctx,
                               uint8_t* alive_counter) {
    CAN_Frame_t frame;
    uint8_t encoded[UART_CAN_MAX_ENCODED_SIZE];
    vcu_calculate_motor_pwm(ctx);
    if (!vcu_build_motor_command(ctx, *alive_counter, &frame)) {
        return false;
    }
    const int length = uart_encode_frame(encoded, (int)sizeof(encoded), &frame);
    if (length <= 0 || write(serial_port, encoded, (size_t)length) != length) {
        return false;
    }
    ipc_can_trace(stdout, "UART_TX", &frame);
    *alive_counter = (uint8_t)((*alive_counter + 1U) & 0x0FU);
    return true;
}

static bool simulation_expect(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "[FAIL] %s\n", message);
        return false;
    }
    return true;
}

static bool simulation_process(VCU_VehicleContext_t* ctx, const CAN_Frame_t* frame) {
    ipc_can_trace(stdout, "VCAN_RX", frame);
    return vcu_process_frame(ctx, frame);
}

static int run_simulation(void) {
    VCU_VehicleContext_t ctx;
    CAN_Frame_t frame;
    bool passed = true;
    vcu_init(&ctx);

    printf("IPC CAN database simulation\n");
    printf("DBC: ../../can_database/ipc_virtual_can.dbc\n\n");

    EcuEnergyStatus_t energy = {.battery_soc = 86, .alive_counter = 0};
    passed &= simulation_expect(ipc_can_pack_ecu_energy_status(&frame, &energy),
                                "encode ECU_ENERGY_STATUS");
    passed &= simulation_expect(simulation_process(&ctx, &frame),
                                "decode ECU_ENERGY_STATUS");

    EcuDiagStatus_t diagnostics = {.active_dtc = 0, .fault_flags = 0, .alive_counter = 0};
    passed &= simulation_expect(ipc_can_pack_ecu_diag_status(&frame, &diagnostics),
                                "encode ECU_DIAG_STATUS");
    passed &= simulation_expect(simulation_process(&ctx, &frame),
                                "decode ECU_DIAG_STATUS");

    EcuDriveStatus_t drive = {
        .pedal_percent = 0,
        .speed_kmh = 0,
        .gear = IPC_GEAR_D,
        .applied_pwm = 0,
        .alive_counter = 0,
    };
    passed &= simulation_expect(ipc_can_pack_ecu_drive_status(&frame, &drive),
                                "encode initial ECU_DRIVE_STATUS");
    passed &= simulation_expect(simulation_process(&ctx, &frame),
                                "select Drive while pedal released");

    drive.pedal_percent = 42;
    drive.speed_kmh = 63;
    drive.applied_pwm = 107;
    drive.alive_counter = 1;
    passed &= simulation_expect(ipc_can_pack_ecu_drive_status(&frame, &drive),
                                "encode Drive telemetry");
    passed &= simulation_expect(simulation_process(&ctx, &frame),
                                "decode Drive telemetry");
    passed &= simulation_expect(vcu_calculate_motor_pwm(&ctx) == 107,
                                "Normal mode PWM for 42 percent pedal is 107");

    CAN_Frame_t motor_command;
    passed &= simulation_expect(vcu_build_motor_command(&ctx, 0, &motor_command),
                                "encode VCU_MOTOR_COMMAND");
    ipc_can_trace(stdout, "VCAN_TX", &motor_command);
    VcuMotorCommand_t decoded_command;
    passed &= simulation_expect(
        ipc_can_unpack_vcu_motor_command(&motor_command, &decoded_command)
            && decoded_command.authorized_pwm == 107
            && decoded_command.expected_gear == IPC_GEAR_D
            && decoded_command.torque_enable,
        "decode Drive motor command");

    drive.pedal_percent = 0;
    drive.speed_kmh = 0;
    drive.gear = IPC_GEAR_R;
    drive.applied_pwm = 0;
    drive.alive_counter = 2;
    passed &= simulation_expect(ipc_can_pack_ecu_drive_status(&frame, &drive)
                                && simulation_process(&ctx, &frame),
                                "select Reverse while pedal released");

    drive.pedal_percent = 100;
    drive.speed_kmh = 20;
    drive.applied_pwm = 43;
    drive.alive_counter = 3;
    passed &= simulation_expect(ipc_can_pack_ecu_drive_status(&frame, &drive)
                                && simulation_process(&ctx, &frame),
                                "decode Reverse telemetry");
    passed &= simulation_expect(vcu_calculate_motor_pwm(&ctx) == 43,
                                "Reverse PWM remains capped at 43");
    passed &= simulation_expect(vcu_build_motor_command(&ctx, 1, &motor_command),
                                "encode Reverse motor command");
    ipc_can_trace(stdout, "VCAN_TX", &motor_command);
    passed &= simulation_expect(!vcu_process_frame(&ctx, &frame),
                                "duplicate alive counter is rejected");

    diagnostics.active_dtc = 0x11;
    diagnostics.alive_counter = 1;
    passed &= simulation_expect(ipc_can_pack_ecu_diag_status(&frame, &diagnostics)
                                && simulation_process(&ctx, &frame),
                                "decode low-SOC warning");
    ctx.gear_position = GEAR_D;
    passed &= simulation_expect(vcu_calculate_motor_pwm(&ctx) == 80
                                && ctx.state == VEHICLE_READY,
                                "low-SOC warning keeps Drive enabled with PWM capped at 80");
    passed &= simulation_expect(vcu_build_motor_command(&ctx, 2, &motor_command)
                                && ipc_can_unpack_vcu_motor_command(&motor_command, &decoded_command)
                                && decoded_command.torque_enable,
                                "low-SOC warning permits torque");
    ctx.gear_position = GEAR_R;
    passed &= simulation_expect(vcu_calculate_motor_pwm(&ctx) == 43,
                                "low-SOC warning preserves Reverse limit");

    diagnostics.active_dtc = 0x22;
    diagnostics.fault_flags = ECU_DIAG_FAULT_PRESENT | ECU_DIAG_MOTOR_OVERLOAD;
    diagnostics.alive_counter = 2;
    passed &= simulation_expect(ipc_can_pack_ecu_diag_status(&frame, &diagnostics)
                                && simulation_process(&ctx, &frame),
                                "decode motor-overload DTC");
    passed &= simulation_expect(vcu_calculate_motor_pwm(&ctx) == 0
                                && ctx.state == VEHICLE_FAULT,
                                "DTC forces FAULT and zero PWM");

    energy.battery_soc = 70;
    energy.alive_counter = 1;
    ipc_can_pack_ecu_energy_status(&frame, &energy);
    frame.data[0] ^= 0x01U;
    ipc_can_trace(stdout, "BAD_CRC", &frame);
    passed &= simulation_expect(!vcu_process_frame(&ctx, &frame),
                                "corrupted E2E CRC is rejected");

    uint8_t encoded[UART_CAN_MAX_ENCODED_SIZE];
    const int encoded_length = uart_encode_frame(encoded, (int)sizeof(encoded), &motor_command);
    UartFrameParser_t parser;
    CAN_Frame_t decoded_frame;
    bool decoded = false;
    uart_parser_init(&parser);
    for (int index = 0; index < encoded_length; ++index) {
        decoded = uart_parser_feed(&parser, encoded[index], &decoded_frame) || decoded;
    }
    passed &= simulation_expect(decoded
                                && decoded_frame.can_id == motor_command.can_id
                                && decoded_frame.can_dlc == motor_command.can_dlc
                                && memcmp(decoded_frame.data, motor_command.data,
                                          motor_command.can_dlc) == 0,
                                "UART transport preserves the canonical CAN frame");

    encoded[6] ^= 0x01U;
    uart_parser_init(&parser);
    decoded = false;
    for (int index = 0; index < encoded_length; ++index) {
        decoded = uart_parser_feed(&parser, encoded[index], &decoded_frame) || decoded;
    }
    passed &= simulation_expect(!decoded, "corrupted UART transport CRC is rejected");

    printf("\n");
    vcu_print_status(&ctx);
    printf("%s: virtual CAN simulation\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : 2;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "--simulate") == 0) {
        return run_simulation();
    }

    const char* serial_path = (argc > 1) ? argv[1] : DEFAULT_SERIAL_PORT;
    const int serial_port = open(serial_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_port < 0) {
        fprintf(stderr, "Error: unable to open %s: %s\n", serial_path, strerror(errno));
        return 1;
    }
    if (configure_serial(serial_port) != 0) {
        fprintf(stderr, "Error: unable to configure %s: %s\n", serial_path, strerror(errno));
        close(serial_port);
        return 1;
    }

    VCU_VehicleContext_t ctx;
    UartFrameParser_t parser;
    uint8_t motor_counter = 0;
    long last_status_ms = 0;
    vcu_init(&ctx);
    uart_parser_init(&parser);

    printf("IPC VCU virtual CAN console on %s @ 9600 baud\n", serial_path);
    while (1) {
        uint8_t inbound[64];
        const ssize_t length = read(serial_port, inbound, sizeof(inbound));
        if (length > 0) {
            for (ssize_t index = 0; index < length; ++index) {
                CAN_Frame_t frame;
                if (uart_parser_feed(&parser, inbound[index], &frame)) {
                    ipc_can_trace(stdout, "UART_RX", &frame);
                    if (vcu_process_frame(&ctx, &frame)
                        && frame.can_id == CAN_ID_ECU_DRIVE_STATUS) {
                        (void)send_motor_command(serial_port, &ctx, &motor_counter);
                    }
                }
            }
        }

        const long now = monotonic_ms();
        if (now - last_status_ms >= 500) {
            last_status_ms = now;
            vcu_calculate_motor_pwm(&ctx);
            vcu_print_status(&ctx);
        }
        usleep(1000);
    }
}
