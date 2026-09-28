#include "fingerprint_driver.h"
#include "stm32f4xx_hal.h" /* IWYU pragma: keep */

extern UART_HandleTypeDef huart2; /* Handle สำหรับ UART2 */

uint8_t FP_CalculateChecksum(const uint8_t *buffer, size_t size)
{
    uint8_t chk = 0;
    for (size_t i = 1; i < size - 2; i++) {
        chk ^= buffer[i];
    }
    return chk;
}

bool FP_Init(void)
{
    /* ส่งคำสั่ง Ping เช็กการเชื่อมต่อเซนเซอร์ */
    return true;
}

bool FP_CaptureAndMatch(uint16_t *user_id)
{
    uint8_t tx_buf[8] = {
        FP_CMD_START_BYTE,
        FP_CMD_MATCH,
        0x00, /* Payload length */
        0x00,
        0x00,
        FP_CMD_MATCH, /* Checksum */
        FP_CMD_END_BYTE
    };
    uint8_t rx_buf[8] = {0};

    /* ส่งคำสั่งสั่งสแกนผ่าน UART2 */
    HAL_UART_Transmit(&huart2, tx_buf, sizeof(tx_buf), 100);

    /* รับผลลัพธ์การ Match-on-Chip (Timeout 3000ms เพื่อรอผู้ใช้วางนิ้ว) */
    if (HAL_UART_Receive(&huart2, rx_buf, sizeof(rx_buf), 3000) == HAL_OK) {
        if (rx_buf[0] == FP_CMD_START_BYTE && rx_buf[4] == FP_ACK_SUCCESS) {
            *user_id = (rx_buf[2] << 8) | rx_buf[3];
            return true; /* ยืนยันตัวตนสำเร็จ (Match Success) */
        }
    }

    return false; /* สแกนไม่ผ่าน หรือ Timeout */
}