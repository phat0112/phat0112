#ifndef TCS34725_H
#define TCS34725_H

#define TCS34725_I2C_ADDRESS         (0x29)

/* TCS34725 registers */
#define TCS34725_COMMAND_BIT         (0x80)
#define TCS34725_ENABLE              (0x00)
#define TCS34725_ATIME               (0x01)
#define TCS34725_WTIME               (0x03)
#define TCS34725_AILTL               (0x04)
#define TCS34725_AILTH               (0x05)
#define TCS34725_AIHTL               (0x06)
#define TCS34725_AIHTH               (0x07)
#define TCS34725_PERS                (0x0C)
#define TCS34725_CONFIG              (0x0D)
#define TCS34725_CONTROL             (0x0F)
#define TCS34725_ID                  (0x12)
#define TCS34725_STATUS              (0x13)
#define TCS34725_CDATAL              (0x14)
#define TCS34725_CDATAH              (0x15)
#define TCS34725_RDATAL              (0x16)
#define TCS34725_RDATAH              (0x17)
#define TCS34725_GDATAL              (0x18)
#define TCS34725_GDATAH              (0x19)
#define TCS34725_BDATAL              (0x1A)
#define TCS34725_BDATAH              (0x1B)

/* Enable register bits */
#define TCS34725_ENABLE_AIEN         (0x10)  /* RGBC Interrupt Enable */
#define TCS34725_ENABLE_WEN          (0x08)  /* Wait enable */
#define TCS34725_ENABLE_AEN          (0x02)  /* RGBC Enable */
#define TCS34725_ENABLE_PON          (0x01)  /* Power on */

/* Integration time values */
#define TCS34725_INTEGRATIONTIME_2_4MS  (0xFF)  /* 2.4ms - 1 cycle    - Max Count: 1024 */
#define TCS34725_INTEGRATIONTIME_24MS   (0xF6)  /* 24ms  - 10 cycles  - Max Count: 10240 */
#define TCS34725_INTEGRATIONTIME_50MS   (0xEB)  /* 50ms  - 20 cycles  - Max Count: 20480 */
#define TCS34725_INTEGRATIONTIME_101MS  (0xD5)  /* 101ms - 42 cycles  - Max Count: 43008 */
#define TCS34725_INTEGRATIONTIME_154MS  (0xC0)  /* 154ms - 64 cycles  - Max Count: 65535 */
#define TCS34725_INTEGRATIONTIME_700MS  (0x00)  /* 700ms - 256 cycles - Max Count: 65535 */

/* Gain values */
#define TCS34725_GAIN_1X             (0x00)  /* No gain */
#define TCS34725_GAIN_4X             (0x01)  /* 4x gain */
#define TCS34725_GAIN_16X            (0x02)  /* 16x gain */
#define TCS34725_GAIN_60X            (0x03)  /* 60x gain */

struct tcs34725_data {
    u16 red;
    u16 green;
    u16 blue;
    u16 clear;
};

#endif /* TCS34725_H */
