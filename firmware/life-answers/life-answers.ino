// =====================================================================
//  Life Answers · 摇摇答案机
//  ESP32-C3 SuperMini + 2.13" WF0213T1PCZ04 (UC8151, 104x212) + MPU6050
//
//  功能:
//    开机 / 摇晃 -> 随机显示一条答案(英文上 / 中文下, 共 152 条)
//    答案页摇晃 -> 立即回封面; 封面摇晃 -> 新答案
//    答案停留 6s -> 回封面; 封面静置 2min -> 深睡(2s 轮询唤醒)
//    插着 USB 时永不深睡, 方便调试烧录
//
//  硬件接线 (C3):  屏 SCK=2 SDA=3 CS1=4 DC=5 RST=6 BUSY=7
//                  MPU6050 I2C SCL=0 SDA=1
//  面板时序与波形来源见 EPaper_213_Driver.ino 文件头说明
// =====================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <Wire.h>
#if defined(CONFIG_IDF_TARGET_ESP32C3)
#include "driver/usb_serial_jtag.h"
#endif
#include <vector>
#include "EPD.h"
#include "answers.h"
#include "cover.h"

void hardwareSpi(uint8_t data);   // 定义在 EPD.ino

// 面板 (竖屏原始方向) 与画布 (横排方向)
#define PANEL_W 104
#define PANEL_H 212
#define CW 212     // 画布宽 = 面板高
#define CH 104     // 画布高 = 面板宽
#define MARGIN 6

GFXcanvas1 canvas(CW, CH);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// 2bit 灰度编码: nibble 0xF=白, 0x0=黑
static const uint8_t g_bw2grey[16] = {0x00,0x03,0x0C,0x0F,0x30,0x33,0x3C,0x3F,
                                      0xC0,0xC3,0xCC,0xCF,0xF0,0xF3,0xFC,0xFF};

// ---------------------------------------------------------------------
// 折行
// ---------------------------------------------------------------------
std::vector<String> wrapUtf8(const char *text, int maxWidth) {
  std::vector<String> lines;
  String line;
  const char *p = text;
  std::vector<String> tokens;
  while (*p) {
    String tok;
    if (*p == ' ') { tokens.push_back(" "); p++; continue; }
    int len = 1;
    uint8_t c = (uint8_t)*p;
    if      (c >= 0xF0) len = 4;
    else if (c >= 0xE0) len = 3;
    else if (c >= 0xC0) len = 2;
    while (len-- && *p) tok += *p++;
    tokens.push_back(tok);
  }
  for (auto &tok : tokens) {
    if (tok == " ") { if (line.length()) line += " "; continue; }
    String test = line + tok;
    if (u8g2.getUTF8Width(test.c_str()) > maxWidth && line.length()) {
      if (line.endsWith(" ")) line.remove(line.length() - 1);
      lines.push_back(line);
      line = tok;
    } else {
      line = test;
    }
  }
  if (line.length()) {
    if (line.endsWith(" ")) line.remove(line.length() - 1);
    lines.push_back(line);
  }
  return lines;
}

std::vector<String> wrapAscii(const char *text, int maxWidth) {
  std::vector<String> lines;
  String line, word;
  auto asciiWidth = [](const String &s) {
    int16_t x1, y1; uint16_t w, h;
    canvas.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return (int)w;
  };
  for (const char *p = text; ; p++) {
    char c = *p;
    if (c == ' ' || c == '\0') {
      if (word.length()) {
        String test = line.length() ? line + " " + word : word;
        if (asciiWidth(test) > maxWidth && line.length()) {
          lines.push_back(line);
          line = word;
        } else {
          line = test;
        }
        word = "";
      }
      if (c == '\0') break;
    } else {
      word += c;
    }
  }
  if (line.length()) lines.push_back(line);
  return lines;
}

// ---------------------------------------------------------------------
// 答案页渲染 (英文上 / 中文下, 无分隔线)
// ---------------------------------------------------------------------
void renderAnswer(const char *en, const char *zh) {
  canvas.fillScreen(1);
  u8g2.begin(canvas);
  u8g2.setFontMode(1);
  u8g2.setForegroundColor(0);
  u8g2.setBackgroundColor(1);
  canvas.setTextColor(0);

  const int usable = CW - 2 * MARGIN;

  canvas.setFont(&FreeSerifBold9pt7b);
  auto enLines = wrapAscii(en, usable);
  const int enLineH = 15;
  int enH = (int)enLines.size() * enLineH;

  // 中文字号自适应: 16 -> 14 -> 13 -> 12
  struct ZhFont { const uint8_t *f; int lh; };
  static const ZhFont zfs[4] = {
    {(const uint8_t*)u8g2_font_wqy16_t_gb2312, 21},
    {(const uint8_t*)u8g2_font_wqy14_t_gb2312, 19},
    {(const uint8_t*)u8g2_font_wqy13_t_gb2312, 17},
    {(const uint8_t*)u8g2_font_wqy12_t_gb2312, 15},
  };
  const int gap = 8;
  int zi = 0;
  std::vector<String> zhLines;
  int zhH = 0;
  for (; zi < 4; zi++) {
    u8g2.setFont(zfs[zi].f);
    zhLines = wrapUtf8(zh, usable);
    zhH = (int)zhLines.size() * zfs[zi].lh;
    if (enH + gap + zhH <= CH - 2 * MARGIN - 2) break;
  }

  int total = enH + gap + zhH;
  int y0 = MARGIN + 2 + (CH - 2 * MARGIN - 4 - total) / 2;
  if (y0 < MARGIN) y0 = MARGIN;

  canvas.setFont(&FreeSerifBold9pt7b);
  int y = y0 + 12;
  for (auto &l : enLines) {
    int16_t x1, y1; uint16_t w, h;
    canvas.getTextBounds(l, 0, 0, &x1, &y1, &w, &h);
    canvas.setCursor((CW - (int)w) / 2, y);
    canvas.print(l);
    y += enLineH;
  }

  u8g2.setFont(zfs[zi].f);
  y = y0 + enH + gap + zfs[zi].lh - 4;
  for (auto &l : zhLines) {
    int x = (CW - u8g2.getUTF8Width(l.c_str())) / 2;
    u8g2.setCursor(x, y);
    u8g2.print(l);
    y += zfs[zi].lh;
  }

  canvas.drawRoundRect(1, 1, CW - 3, CH - 3, 3, 0);
}

// ---------------------------------------------------------------------
// 封面: 大标题"人生答案"(2x放大) + 英文副标题 + 中文标语
// ---------------------------------------------------------------------
void renderCover() {
  canvas.fillScreen(1);

  // 顶部英文: 用 5x7 点阵, 一行放得下
  canvas.setFont();                 // 默认 5x7 内置字体
  canvas.setTextSize(1);
  const char *en = "LIFE ANSWERS";
  int16_t w = (int)strlen(en) * 6;
  canvas.setCursor((CW - w) / 2, 7);
  canvas.print(en);

  // 大标题条带 (人生答案 + 书图标, 位图)
  canvas.drawBitmap((CW - STRIP_W) / 2, 24, STRIP_BITS, STRIP_W, STRIP_H, 0);

  // 底部中文标语 (字间留空)
  u8g2.begin(canvas);
  u8g2.setFontMode(1);
  u8g2.setForegroundColor(0);
  u8g2.setBackgroundColor(1);
  u8g2.setFont((const uint8_t*)u8g2_font_wqy16_t_gb2312);
  const char *tag = "给困惑一个方向";
  const int cw = 16, sp = 5;
  int totalW = 7 * cw + 6 * sp;
  int x = (CW - totalW) / 2;
  const char *p = tag;
  while (*p) {
    char buf[4];
    int len = 1;
    uint8_t c = (uint8_t)*p;
    if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
    for (int i = 0; i < len && *p; i++) buf[i] = *p++;
    buf[len] = 0;
    u8g2.setCursor(x, CH - 10);
    u8g2.print(buf);
    x += cw + sp;
  }
}

// ---------------------------------------------------------------------
// 送屏 (旋转90°, 灰度路径)
// 面板(px,py) <- 画布(canvas_x=py, canvas_y=CH-1-px)
// ---------------------------------------------------------------------
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

void flushPanel() {
  EPaper_Init();          // 复位 + 初始化 + LUT
  sendCanvasRotated();    // 显存
  EPaper_refresh();       // 0x12 + REFRESH_WAIT_MS
  EPaper_PowerOff();
}

void showAnswer(int idx) {
  Serial.printf("\n[答案 #%d] %s | %s\n", idx + 1, ANSWERS[idx].en, ANSWERS[idx].zh);
  renderAnswer(ANSWERS[idx].en, ANSWERS[idx].zh);
  flushPanel();
  Serial.println("[答案显示完成]");
}

void showCover() {
  Serial.println("\n[封面] LIFE ANSWERS");
  renderCover();
  flushPanel();
  Serial.println("[封面显示完成]");
}

// ---------------------------------------------------------------------
//  MPU6050 摇晃检测 (I2C)
// ---------------------------------------------------------------------
#define MPU_ADDR      0x68
#define SAMPLE_MS     10       // 采样间隔: 10ms 一次, 快速甩动也不漏采
#define SHAKE_G       1.20f    // |a| 阈值 (静止约 1.0g, 越小越灵敏)
#define JERK_G        0.35f    // 相邻采样突变阈值 (轻甩也能触发)
#define QUIET_MS      6000UL   // 答案显示 6 秒后回封面
#define SLEEP_MS      120000UL // 封面静止 2 分钟后深睡
#define POLL_S        2        // 深睡定时唤醒周期 (秒)
#define WAKE_WIN_MS   300      // 唤醒后补采窗口
#define COOLDOWN_MS   1200UL   // 刷屏后忽略摇晃, 防止一次甩动连触发

static uint32_t lastShakeMs = 0;

uint8_t mpuReadReg(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)1) != 1) return 0;
  return Wire.read();
}

void mpuWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

void mpuInit() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  mpuWriteReg(0x6B, 0x00);     // 退出睡眠
  delay(20);
  mpuWriteReg(0x1C, 0x10);     // ±8g
  mpuWriteReg(0x19, 0x04);     // DLPF
  // 硬件运动检测 (MOT): 睡着期间的晃动会被锁存到 INT_STATUS, 醒来一读就知道
  mpuWriteReg(0x1F, 0x08);     // MOT_THR = 8 x 32mg = 256mg
  mpuWriteReg(0x20, 0x20);     // MOT_DUR = 32ms
  mpuWriteReg(0x69, 0x3F);     // MOT_DETECT_CTRL
  mpuWriteReg(0x38, 0x40);     // INT_ENABLE: MOT
  delay(20);
  mpuReadReg(0x3A);            // 清一次中断状态
  Serial.println("[MPU6050] 初始化完成");
}

float readMag() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return 0;
  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  // ±8g 量程 = 4096 LSB/g
  return sqrt((float)ax*ax + (float)ay*ay + (float)az*az) / 4096.0f;
}

// 一次判定: |a| 超阈值 或 相邻采样突变 => 认为在甩
bool shakeStep(float &prev) {
  float m = readMag();
  if (m <= 0.01f) return false;                 // I2C 读失败, 不当作摇晃
  bool hit = (m > SHAKE_G) || (fabsf(m - prev) > JERK_G);
  prev = m;
  return hit;
}

bool detectShake(uint32_t windowMs) {
  float prev = readMag();
  if (prev <= 0.01f) prev = 1.0f;
  uint32_t t0 = millis();
  while (millis() - t0 < windowMs) {
    if (shakeStep(prev)) return true;
    delay(SAMPLE_MS);
  }
  return false;
}

// 深睡醒来: 运动中断锁存 或 短窗口补采, 任一命中就认为有甩动
bool wakeShakeCheck() {
  if (mpuReadReg(0x3A) & 0x40) return true;     // MOT_INT 锁存位
  return detectShake(WAKE_WIN_MS);
}

// USB 是否插在电脑上 (C3 的 USB-Serial/JTAG)
bool usbPlugged() {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  return usb_serial_jtag_is_connected();
#else
  return false;
#endif
}

void goDeepSleep() {
  Serial.printf("[深睡] 屏幕断电, %d 秒定时唤醒检查摇晃\n", POLL_S);
  EPaper_PowerOff();
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)POLL_S * 1000000ULL);
  esp_deep_sleep_start();
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n===== 人生答案 v4 (灵敏摇晃 + 6s 回封面 + 2s 轮询深睡) =====");

  bool wokeByTimer = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);

  mpuInit();

  if (wokeByTimer) {
    // 定时醒来只确认有没有被甩; 没有就立刻回睡, 不刷屏
    if (!wakeShakeCheck()) {
      Serial.println("[无晃动] 继续深睡");
      Serial.flush();
      esp_sleep_enable_timer_wakeup((uint64_t)POLL_S * 1000000ULL);
      esp_deep_sleep_start();
    }
    Serial.println("[唤醒] 检测到摇晃 -> 随机答案");
  }

  EPD_Init();
  showAnswer(esp_random() % ANSWERS_COUNT);
  lastShakeMs = millis();
}

void loop() {
  static bool     onCover     = false;   // 当前显示的是封面吗
  static float    prev        = 1.0f;    // 上一次 |a|
  static uint32_t ignoreUntil = 0;       // 冷却截止时间

  // 10ms 采样一次, 一甩就能捕捉到
  float m = readMag();
  bool  hit = false;
  if (m > 0.01f) {
    hit  = (m > SHAKE_G) || (fabsf(m - prev) > JERK_G);
    prev = m;
  }

  if (hit && millis() >= ignoreUntil) {
    Serial.printf("[摇晃] |a|=%.2fg -> %s\n", m, onCover ? "随机答案" : "封面");
    prev = 1.0f;
    if (onCover) {
      showAnswer(esp_random() % ANSWERS_COUNT);
      onCover = false;
    } else {
      showCover();
      onCover = true;
    }
    lastShakeMs = millis();
    ignoreUntil = millis() + COOLDOWN_MS;
    return;
  }

  uint32_t idle = millis() - lastShakeMs;

  // 答案页显示满 6 秒 -> 自动回封面
  if (!onCover && idle > QUIET_MS) {
    showCover();
    onCover = true;
    lastShakeMs = millis();
    ignoreUntil = millis() + COOLDOWN_MS;
    return;
  }

  // 封面静止 -> 深睡省电, 每 2 秒醒来检查
  // 插着 USB 调试时不睡, 否则 COM 口会消失, 没法烧录
  if (onCover && idle > SLEEP_MS && !usbPlugged()) {
    goDeepSleep();
  }

  delay(SAMPLE_MS);
}
