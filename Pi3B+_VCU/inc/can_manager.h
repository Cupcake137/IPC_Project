#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <stdint.h>

typedef struct {
    uint8_t  sof;          
    uint16_t can_id;       
    uint8_t  rtr;          
    uint8_t  ide_r0;       
    uint8_t  dlc;          
    uint8_t  data[8];      
    uint16_t crc;          
    uint8_t  ack;          
    uint8_t  eof;          
    uint8_t  counter;      
} __attribute__((packed)) CAN_Frame_t;

uint16_t calculate_CAN_CRC(uint16_t id, uint8_t dlc, uint8_t* data);

#endif