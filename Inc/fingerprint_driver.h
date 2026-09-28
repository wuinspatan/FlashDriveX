#ifndef FINGERPRINT_DRIVER_H
#define FINGERPRINT_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Fingerprint Command Protocol Definitions */
#define FP_CMD_START_BYTE     0xF5
#define FP_CMD_END_BYTE       0xF7

#define FP_CMD_ENROLL         0x01
#define FP_CMD_MATCH          0x03
#define FP_CMD_CANCEL         0x30

#define FP_ACK_SUCCESS        0x00
#define FP_ACK_FAIL           0x01

/* Structure สำหรับ Packet คำสั่งของเซนเซอร์ */
typedef struct {
    uint8_t start;
    uint8_t cmd;
    uint8_t data_len;
    uint8_t payload[16];
    uint8_t checksum;
    uint8_t end;
} FP_Packet_t;

/* Function Prototypes */
bool FP_Init(void);
bool FP_CaptureAndMatch(uint16_t *user_id);
uint8_t FP_CalculateChecksum(const uint8_t *buffer, size_t size);

#endif /* FINGERPRINT_DRIVER_H */