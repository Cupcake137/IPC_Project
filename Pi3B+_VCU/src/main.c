#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include "can_manager.h"
#include "vcu_core.h"

int main() {
    int serial_port = open("/dev/ttyUSB0", O_RDWR | O_NONBLOCK);
    if (serial_port < 0) {
        printf("Error: Unable to open port /dev/ttyUSB0\n");
        return 1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) return 1;
    cfmakeraw(&tty);
    cfsetispeed(&tty, B9600); cfsetospeed(&tty, B9600);
    tcsetattr(serial_port, TCSANOW, &tty);

    VCU_VehicleContext_t vcu_vehicle_state = {0, MODE_NORMAL, 0, 100, 0, 0, 0, 0};
    uint8_t localTxCounter = 0;
    uint8_t raw_rx_buffer[sizeof(CAN_Frame_t)];
    int rx_byte_count = 0;

    fd_set read_fds;
    int max_fd = (serial_port > STDIN_FILENO) ? serial_port : STDIN_FILENO;

    printf("=======================================================================\n");
    printf("--- MODULAR VEHICLE CONTROL UNIT (VCU) ONLINE ---\n");
    printf("Controls: [i] Ignition READY | [e] ECO | [n] NORMAL | [s] SPORT\n");
    printf("=======================================================================\n\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(serial_port, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) break;

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char ch = getchar();
            if (ch == 'i') {
                vcu_vehicle_state.is_ready_to_drive = !vcu_vehicle_state.is_ready_to_drive;
                if(!vcu_vehicle_state.is_ready_to_drive) vcu_vehicle_state.authorized_pwm = 0;
                printf("[HMI EVENT] READY Latch -> %s\n", vcu_vehicle_state.is_ready_to_drive ? "ACTIVE" : "LOCKED");
            } else if (ch == 'e') {
                vcu_vehicle_state.drive_mode = MODE_ECO;
                printf("[HMI EVENT] Drive Profile -> ECO\n");
            } else if (ch == 'n') {
                vcu_vehicle_state.drive_mode = MODE_NORMAL;
                printf("[HMI EVENT] Drive Profile -> NORMAL\n");
            } else if (ch == 's') {
                vcu_vehicle_state.drive_mode = MODE_SPORT;
                printf("[HMI EVENT] Drive Profile -> SPORT\n");
            }
        }

        if (FD_ISSET(serial_port, &read_fds)) {
            uint8_t inbound_byte;
            if (read(serial_port, &inbound_byte, 1) > 0) {
                if (rx_byte_count < (int)sizeof(CAN_Frame_t)) {
                    raw_rx_buffer[rx_byte_count++] = inbound_byte;
                }

                if (rx_byte_count == sizeof(CAN_Frame_t)) {
                    CAN_Frame_t *frame = (CAN_Frame_t*)raw_rx_buffer;
                    uint16_t localCrc = calculate_CAN_CRC(frame->can_id, frame->dlc, frame->data);

                    if (localCrc == frame->crc) {
                        execute_vcu_powertrain_strategy(&vcu_vehicle_state, frame);

                        CAN_Frame_t txFrame;
                        txFrame.sof = 0x00; txFrame.can_id = 0x0100; txFrame.rtr = 0; txFrame.ide_r0 = 0;
                        txFrame.dlc = 1;
                        memset(txFrame.data, 0, 8);
                        txFrame.data[0] = vcu_vehicle_state.authorized_pwm;
                        txFrame.crc = calculate_CAN_CRC(0x0100, 1, txFrame.data);
                        txFrame.ack = 0xFF; txFrame.eof = 0x7F;
                        txFrame.counter = localTxCounter;

                        write(serial_port, (uint8_t*)&txFrame, sizeof(CAN_Frame_t));
                        localTxCounter = (localTxCounter + 1) % 16;

                        rx_byte_count = 0; 
                    } 
                    else {
                        memmove(raw_rx_buffer, raw_rx_buffer + 1, sizeof(CAN_Frame_t) - 1);
                        rx_byte_count--;
                    }
                }
            }
        }
    }

    close(serial_port);
    return 0;
}