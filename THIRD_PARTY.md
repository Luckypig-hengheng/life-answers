# 第三方组件与许可声明 (THIRD PARTY NOTICES)

本项目包含或依赖以下第三方组件，在此致谢并声明其许可条款。

## 编译期依赖（Arduino 库，需用户自行安装）

| 组件 | 用途 | 许可 |
|---|---|---|
| [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32) | ESP32-C3 支持 | LGPL-2.1 |
| [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) | 画布/点阵绘制 | BSD-3-Clause |
| [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) | 中文点阵字体渲染 | BSD-3-Clause |

## 固件内嵌的字体数据

| 字体 | 原始来源 | 许可 | 使用方式 |
|---|---|---|---|
| WenQuanYi bitmap 字体（u8g2 `u8g2_font_wqy*_t_gb2312`） | 文泉驿项目，经 u8g2 库转制 | GPLv3 + 字体嵌入例外 | 编译进固件用于中文答案/封面标语渲染 |
| FreeSerif Bold（Adafruit GFX `FreeSerifBold9pt7b`） | GNU FreeFont，经 Adafruit GFX 字体包转制 | GPLv3 + 字体嵌入例外 | 编译进固件用于英文答案渲染 |

上述两个组件均为"字体嵌入例外"许可：允许把字体（或其渲染结果）嵌入文档/固件中分发，
不受 GPL 传染条款约束。若你需要从源码重新构建本项目，需按各自许可使用这些库。

## 封面位图

`firmware/life-answers/cover.h` 中的标题条带由 `content/gen_cover.py` 使用
[思源黑体 (Source Han Sans SC Bold)](https://github.com/adobe-fonts/source-han-sans)
渲染生成。思源黑体采用 SIL Open Font License 1.1，明确允许自由使用、渲染与再分发。

## 屏幕波形数据

`firmware/life-answers/EPaper_213_Driver.ino` 中 `lut_20 ~ lut_27` 共 8 组刷新波形
数据来自 WF0213T1PCZ04 屏幕厂商随屏提供的示例程序，仅作为该型号屏幕的硬件配置
参数使用。寄存器命令序列依据 UC8151 芯片数据手册的公开时序编写。

## 灵感来源声明

本项目的玩法（"提问后随机获得一句简短回答"）受"翻页占卜书"这类互动书籍形式的启发。
本项目的全部答案文本为本项目原创改写，与 Carol Bolt 所著 *The Book of Answers*
（《答案之书》/《解答之书》）及其他任何在售图书**均无关联、不含其内容、并非其衍生作品**。
