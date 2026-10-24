// =====================================================================
//  RefreshCal · 墨屏刷新等待时间标定
//  每次刷新用不同的等待时间, 画面停留, 甩一下切下一个档位
//  观察每张图: 黑块是否够黑、细条纹是否完整、有无上一条残影
//  找到"最干净里最短的那档", 写回 BookOfAnswers2/EPaper_213_Driver.h
// =====================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <math.h>
#include "EPD.h"
#include "global.h"

void hardwareSpi(uint8_t data);

#define PANEL_W 104
#define PANEL_H 212
#define CW 212
#define CH 104

GFXcanvas1 canvas(CW, CH);

static const uint8_t g_bw2grey[16] = {0x00,0x03,0x0C,0x0F,0x30,0x33,0x3C,0x3F,
                                      0xC0,0xC3,0xCC,0xCF,0xF0,0xF3,0xFC,0xFF};

// 从小到大: 最后一档最短
static const uint16_t WAITS[] = {18000, 9000, 6000, 4500, 3000, 2400, 1800, 1200};
static const int N_WAITS = sizeof(WAITS) / sizeof(WAITS[0]);
static int idx = 0;

// ---------------------------------------------------------------------
void drawCard(int n, uint16_t waitMs) {
  canvas.fillScreen(1);
  canvas.setTextColor(0);

  // 外框
  canvas.drawRect(0, 0, CW, CH, 0);
  canvas.drawRect(1, 1, CW - 2, CH - 2, 0);

  // 顶部标题
  canvas.setTextSize(1);
  canvas.setCursor(6, 6);
  canvas.print("REFRESH CAL");

  // 序号 (大号)
  canvas.setTextSize(5);
  char num[4]; sprintf(num, "%d", n);
  canvas.setCursor(8, 16);
  canvas.print(num);

  // 档位参数
  canvas.setTextSize(2);
  char buf[24]; sprintf(buf, "%dms", waitMs);
  canvas.setCursor(56, 24);
  canvas.print(buf);

  // 右侧实心黑块: 刷新不足会发灰/没画完
  canvas.fillRect(CW - 30, 14, 22, 40, 0);

  // 中间细横条: 检查像素是否逐个到位
  for (int y = 62; y < 78; y += 2) canvas.drawFastHLine(10, y, CW - 20, 0);

  // 底部粗黑条
  canvas.fillRect(10, 84, CW - 20, 8, 0);
}

void sendCanvasRotated() {
  sendCmd(0x10);
  digitalWrite(EPD_DC_PIN, HIGH);
  digitalWrite(EPD_CS_PIN, LOW);
  for (int py = 0; py < PANEL_H; py++) {
    uint8_t row[13] = {0};
    for (int px = 0; px < PANEL_W; px++) {
      if (canvas.getPixel(py, CH - 1 - px))
        row[px >> 3] |= 0x80 >> (px & 7);
    }
    for (int j = 0; j < 13; j++) {
      uint8_t v = row[j];
      hardwareSpi(g_bw2grey[(v >> 4) & 0xF]);
      hardwareSpi(g_bw2grey[v & 0xF]);
    }
  }
  digitalWrite(EPD_CS_PIN, HIGH);
}

void epdRefreshWith(uint16_t waitMs) {
  EPaper_Init();
  sendCanvasRotated();
  Serial.printf("[DRF] 等待 %ums\n", waitMs);
  sendCmd(0x12);
  delay(waitMs);
  EPaper_PowerOff();
}

void showCard() {
  uint16_t w = WAITS[idx];
  Serial.printf("\n===== 第 %d/%d 档: %ums =====\n", idx + 1, N_WAITS, w);
  drawCard(idx + 1, w);
  epdRefreshWith(w);
  Serial.println("[完成] 甩一下看下一档");
}

// ---------------------------------------------------------------------
#define MPU_ADDR 0x68
#define SHAKE_G  1.20f
#define JERK_G   0.35f

void mpuWrite(uint8_t r, uint8_t v) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(r); Wire.write(v);
  Wire.endTransmission();
}

float readMag() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return 0;
  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  return sqrt((float)ax*ax + (float)ay*ay + (float)az*az) / 4096.0f;
}

void waitShakeRelease() {           // 等手停下来, 避免一次甩动跳好几档
  float prev = 1.0f;
  uint32_t t0 = millis();
  while (millis() - t0 < 1200) {
    float m = readMag();
    if (m > 0.01f) prev = m;
    delay(20);
  }
}

void waitShake() {
  float prev = readMag();
  if (prev <= 0.01f) prev = 1.0f;
  while (true) {
    float m = readMag();
    if (m > 0.01f) {
      if (m > SHAKE_G || fabsf(m - prev) > JERK_G) {
        Serial.printf("[摇晃] |a|=%.2fg\n", m);
        return;
      }
      prev = m;
    }
    delay(10);
  }
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n===== RefreshCal 刷新等待标定 =====");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  mpuWrite(0x6B, 0x00);
  delay(20);
  mpuWrite(0x1C, 0x10);
  mpuWrite(0x19, 0x04);

  EPD_Init();
  idx = 0;
  showCard();
}

void loop() {
  waitShake();
  waitShakeRelease();
  idx = (idx + 1) % N_WAITS;
  showCard();
  delay(50);
}
