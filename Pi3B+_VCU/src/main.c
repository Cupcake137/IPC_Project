#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "can_manager.h"
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

static void send_motor_command(int serial_port, VCU_VehicleContext_t* ctx, uint8_t* tx_counter) {
    uint8_t payload = vcu_calculate_motor_pwm(ctx);
    uint8_t tx[16];
    const int n = uart_encode_frame(tx, (int)sizeof(tx), MSG_MOTOR_COMMAND, 1, &payload, tx_counter);
    if (n > 0) {
        (void)write(serial_port, tx, (size_t)n);
    }
}

int main(int argc, char** argv) {
    const char* serial_path = (argc > 1) ? argv[1] : DEFAULT_SERIAL_PORT;
    int serial_port = open(serial_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
    uint8_t tx_counter = 0;
    long last_status_ms = 0;

    vcu_init(&ctx);
    uart_parser_init(&parser);

    printf("=======================================================================\n");
    printf("--- IPC VCU console started on %s @ 9600 baud ---\n", serial_path);
    printf("=======================================================================\n");

    while (1) {
        uint8_t inbound[64];
        const ssize_t n = read(serial_port, inbound, sizeof(inbound));

        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                CAN_Frame_t frame;
                if (uart_parser_feed(&parser, inbound[i], &frame)) {
                    vcu_process_frame(&ctx, &frame);

                    if (frame.id == MSG_PEDAL_SPEED) {
                        send_motor_command(serial_port, &ctx, &tx_counter);
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

    close(serial_port);
    return 0;
}
