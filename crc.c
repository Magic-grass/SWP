//
// Created by Magic_Grass on 2023/10/8.
//

#include <stdio.h>
#include "crc.h"

#define CRC16_POLY 0xA001

//CRC16校验和
uint16_t crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC16_POLY;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return crc;
}


