# -*- coding: utf-8 -*-
"""生成封面标题条带 (人生答案 + 书图标) -> firmware/life-answers/cover.h
字体: 思源黑体 Source Han Sans SC Bold (SIL OFL 1.1), 可自由分发与嵌入。
用法: python gen_cover.py
"""
import math, os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                       # life-answers/
FONT = os.path.join(HERE, "fonts", "SourceHanSansSC-Bold.otf")
OUT  = os.path.join(ROOT, "firmware", "life-answers", "cover.h")
PREVIEW = os.path.join(HERE, "cover_strip_preview.png")

font = ImageFont.truetype(FONT, 38)
txt = "人生答案"
bbox = font.getbbox(txt)
tw, th = bbox[2]-bbox[0], bbox[3]-bbox[1]
title = Image.new("L", (tw+4, th+4), 255)
ImageDraw.Draw(title).text((2-bbox[0], 2-bbox[1]), txt, font=font, fill=0)

# 开本书图标 (线条) + 右页五角星
iw, ih = 42, 38
icon = Image.new("L", (iw, ih), 255)
di = ImageDraw.Draw(icon)
cx = iw//2
di.line([(cx,11),(4,6),(4,31),(cx,36)], fill=0, width=2, joint="curve")
di.line([(cx,11),(iw-4,6),(iw-4,31),(cx,36)], fill=0, width=2, joint="curve")
di.line([(cx,11),(cx,36)], fill=0, width=1)
for y0 in (16,22,28):
    di.line([(9,y0),(cx-5,y0+1)], fill=0, width=1)
    di.line([(cx+5,y0+1),(iw-9,y0)], fill=0, width=1)
sx, sy, R, r = iw-15, 20, 6, 2.4
pts = []
for k in range(10):
    ang = -math.pi/2 + k*math.pi/5
    rad = R if k%2==0 else r
    pts.append((sx+rad*math.cos(ang), sy+rad*math.sin(ang)))
di.polygon(pts, fill=0)

gap = 7
W = title.width + gap + iw
H = max(title.height, ih) + 2
strip = Image.new("L", (W, H), 255)
strip.paste(title, (0, (H-title.height)//2))
strip.paste(icon, (title.width+gap, (H-ih)//2))
bw = strip.point(lambda p: 0 if p < 150 else 255)
bw.resize((W*3, H*3), Image.NEAREST).save(PREVIEW)

px = bw.load()
data = []
for y in range(H):
    for xb in range((W+7)//8):
        b = 0
        for i in range(8):
            x = xb*8+i
            if x < W and px[x,y] == 0:
                b |= 0x80 >> i
        data.append(b)

out = ["#pragma once",
       "// 封面标题条带 (人生答案 + 开本书图标) %dx%d, 1=黑" % (W, H),
       "// 由 content/gen_cover.py 用思源黑体 (SIL OFL 1.1) 渲染生成, 勿手改",
       "#define STRIP_W %d" % W, "#define STRIP_H %d" % H,
       "const uint8_t STRIP_BITS[%d] = {" % len(data)]
for i in range(0, len(data), 14):
    out.append("  " + ",".join("0x%02X" % b for b in data[i:i+14]) + ",")
out.append("};")
os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, "w").write("\n".join(out))
print("strip", (W, H), "bytes", len(data), "->", OUT)
