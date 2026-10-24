# -*- coding: utf-8 -*-
"""answers_v2.txt -> firmware/life-answers/answers.h (C 数组)
用法: python gen_answers_h.py
"""
import io, os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "answers_v2.txt")
OUT = os.path.join(HERE, "..", "firmware", "life-answers", "answers.h")

items = []
for line in io.open(SRC, encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    en, zh, cat = [p.strip() for p in line.split("|")]
    items.append((en, zh, cat))

def cesc(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')

rows = []
for en, zh, _ in items:
    rows.append('  {"%s", "%s"},' % (cesc(en), cesc(zh)))

out = """#pragma once
// =====================================================================
//  答案库 · %d 条中英双语答案 (v2, 本项目原创改写)
//  由 content/gen_answers_h.py 从 answers_v2.txt 自动生成, 勿手改
//  en: 英文(显示在上方)   zh: 中文(显示在下方)
// =====================================================================

#define ANSWERS_COUNT %d

struct Answer {
  const char *en;
  const char *zh;
};

static const Answer ANSWERS[ANSWERS_COUNT] = {
%s
};
""" % (len(items), len(items), "\n".join(rows))

io.open(OUT, "w", encoding="utf-8", newline="\n").write(out)
print("answers.h:", len(items), "items ->", OUT)
