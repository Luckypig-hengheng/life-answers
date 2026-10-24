# -*- coding: utf-8 -*-
"""把 answers_v2.txt 生成一份可浏览的评审页 (HTML)"""
import io, os, html

HERE = os.path.dirname(os.path.abspath(__file__))

SRC = os.path.join(HERE, "answers_v2.txt")
OUT = os.path.join(HERE, "answers_preview.html")

CATS = {
    "YES":     ("肯定", "#c0392b"),
    "NO":      ("否定", "#2c3e50"),
    "WAIT":    ("时机", "#b7791f"),
    "ACT":     ("行动", "#1f6f43"),
    "LETGO":   ("放下", "#6b4c9a"),
    "INSIGHT": ("自省", "#1f4e79"),
    "REL":     ("关系", "#a0522d"),
    "COMFORT": ("安慰", "#c2567a"),
    "CHANGE":  ("变化", "#2f6f73"),
}

items = []
for line in io.open(SRC, encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    en, zh, cat = [p.strip() for p in line.split("|")]
    items.append((en, zh, cat))

counts = {}
for _, _, c in items:
    counts[c] = counts.get(c, 0) + 1

chips = "".join(
    '<button class="chip" data-c="%s"><i style="background:%s"></i>%s <b>%d</b></button>' % (
        c, CATS[c][1], CATS[c][0], counts.get(c, 0))
    for c in CATS if counts.get(c)
)

cards = []
for i, (en, zh, cat) in enumerate(items, 1):
    label, color = CATS[cat]
    cards.append(
        '<div class="card" data-c="%s">'
        '<div class="top"><span class="no">%03d</span>'
        '<span class="tag" style="color:%s;border-color:%s">%s</span></div>'
        '<div class="en">%s</div><div class="zh">%s</div></div>' % (
            cat, i, color, color, label, html.escape(en), html.escape(zh))
    )

HTML = """<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>人生答案 · 答案清单 v2</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:#f6f5f2;color:#1c1c1a;
     font-family:-apple-system,"PingFang SC","Microsoft YaHei",Segoe UI,sans-serif}
.wrap{max-width:1180px;margin:0 auto;padding:38px 26px 64px}
h1{font-size:26px;margin:0 0 6px;letter-spacing:.5px;font-weight:600}
.sub{color:#8b8880;font-size:13.5px;line-height:1.85;margin:0 0 4px}
.sub b{color:#1c1c1a;font-weight:600}
.rule{height:1px;background:#e2e0da;margin:22px 0 18px}
.chips{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:22px}
.chip{display:inline-flex;align-items:center;gap:6px;background:#fff;border:1px solid #e2e0da;
      border-radius:999px;padding:6px 13px;font-size:12.5px;color:#444;cursor:pointer;
      transition:.15s;font-family:inherit}
.chip:hover{border-color:#c9c6be}
.chip.on{background:#1c1c1a;color:#fff;border-color:#1c1c1a}
.chip i{width:7px;height:7px;border-radius:50%;display:inline-block}
.chip b{font-weight:500;opacity:.6}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(268px,1fr));gap:14px}
.card{background:#fff;border:1px solid #e8e6e0;border-radius:10px;padding:15px 16px 16px;
      min-height:112px;display:flex;flex-direction:column;transition:.18s}
.card:hover{border-color:#cfccc3;box-shadow:0 2px 10px rgba(0,0,0,.045)}
.card.hide{display:none}
.top{display:flex;justify-content:space-between;align-items:center;margin-bottom:11px}
.no{font-size:11.5px;color:#b6b2a8;letter-spacing:1px;font-variant-numeric:tabular-nums}
.tag{font-size:10.5px;border:1px solid;border-radius:4px;padding:1px 5px;opacity:.85}
.en{font-family:Georgia,"Times New Roman",serif;font-size:15.5px;line-height:1.42;
    font-weight:600;color:#141412;margin-bottom:7px}
.zh{font-size:13.5px;color:#6d6a63;line-height:1.6}
footer{margin-top:34px;color:#a09c93;font-size:12px;line-height:1.9}
</style></head><body><div class="wrap">
<h1>人生答案 · 答案清单 v2</h1>
<p class="sub">共 <b>@@N@@</b> 条 ｜ 英文上、中文下，已按屏幕排版约束筛选（英文 ≤46 字符、中文 ≤18 字）</p>
<p class="sub">看完告诉我：<b>删掉多少号</b>、<b>哪类要更多</b>，我按最终清单生成 answers.h 并烧录。</p>
<div class="rule"></div>
<div class="chips">@@CHIPS@@</div>
<div class="grid" id="g">@@CARDS@@</div>
<footer>点击上方分类可筛选 ｜ 再次点击取消筛选</footer>
</div>
<script>
document.querySelectorAll('.chip').forEach(function(b){
  b.onclick=function(){
    var on=b.classList.contains('on');
    document.querySelectorAll('.chip').forEach(function(x){x.classList.remove('on')});
    document.querySelectorAll('.card').forEach(function(c){c.classList.remove('hide')});
    if(!on){b.classList.add('on');
      document.querySelectorAll('.card').forEach(function(c){
        if(c.dataset.c!==b.dataset.c) c.classList.add('hide');});}
  };
});
</script></body></html>
"""
HTML = HTML.replace("@@N@@", str(len(items))).replace("@@CHIPS@@", chips).replace("@@CARDS@@", "".join(cards))

io.open(OUT, "w", encoding="utf-8", newline="\n").write(HTML)
print("items:", len(items), "->", OUT)
