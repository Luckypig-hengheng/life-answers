#ifndef __EPAPER_213_DRIVER_H
#define __EPAPER_213_DRIVER_H

#include <Arduino.h>
#include "global.h"
#include "EPD.h"


#define     WIDTH 104
#define     HEIGHT 212
#define     power_on_time 20
#define     full_refresh_time 7500
// DRF 之后的固定等待: 本板 BUSY 恒高不可用, 只能靠延时。
// 2026-10-18 实测标定(RefreshCal 固件): 4500ms 画面干净, 3000ms 发灰。
// 取 4600ms 留余量; 若日后出现发灰/残影, 回调到 6000~9000。
#define     REFRESH_WAIT_MS 4600

// #define SET_DC_LOW GPIO_ResetBits(GPIOB, GPIO_Pin_11)
// #define	SET_CS_LOW GPIO_ResetBits(GPIOB, GPIO_Pin_12)
// #define SET_DC_HIGH GPIO_SetBits(GPIOB, GPIO_Pin_11)
// #define	SET_CS_HIGH GPIO_SetBits(GPIOB, GPIO_Pin_12)
// #define	SET_RST_LOW GPIO_ResetBits(GPIOB, GPIO_Pin_1)
// #define	SET_RST_HIGH GPIO_SetBits(GPIOB, GPIO_Pin_1)
// #define GET_BUSY_STATUS GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10)

void EPaper_WriteBWImage(const uint8_t* black,uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool invert, bool mirror_y);
void EPaper_WriteGreyImage(const uint8_t* Grey,uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool invert, bool mirror_y);

#endif