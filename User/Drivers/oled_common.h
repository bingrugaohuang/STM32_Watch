#ifndef OLED_COMMON_H
#define OLED_COMMON_H

#define OLED_I2C_ADDR  0x3C // OLED I2C 地址（7 位地址，左移 1 位后最低位为 R/W 位）
#define OLED_CTRL_CMD  0x00 // OLED 控制字节：表示后续字节为命令
#define OLED_CTRL_DATA 0x40 // OLED 控制字节：表示后续字节为数据

#endif /* OLED_COMMON_H */