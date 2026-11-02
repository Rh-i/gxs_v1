#!/usr/bin/env python3
"""三全向轮底盘布局图（只画图，不参与控制）。

改下面的 WHEELS（编号 + 方位角）或 ARROW_SIGN 后重新运行即可：
    python3 omni3_layout.py          # 需要 matplotlib
输出：omni3_layout.png / omni3_layout.svg（与本文件同目录）

约定（图上已标注，实际以你标注为准）：
    +x = 右，+y = 前（俯视，从底盘正上方看下去）
    方位角 θ 从 +x 起逆时针为正
    **前向 = M1 与 M2 的中线**，M3 在正后方
    橙色箭头 = 该轮「正转」时驱动底盘的方向（t = -(-sinθ, cosθ)，ARROW_SIGN=+1 则反向）
"""

import math
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import Circle  # noqa: E402

plt.rcParams["font.sans-serif"] = ["DengXian", "SimHei", "Noto Sans CJK SC", "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

# ---------------- 可改参数 ----------------
# 前向 = M1/M2 的中线 ⇒ M1、M2 关于 +y 对称（±60°），M3 在正后方
WHEELS = [("M1", 30.0), ("M2", 150.0), ("M3", 270.0)]  # (编号, 方位角°)
R = 1.0          # 轮心到车体中心的距离（画图比例，真实值 cm 自己换算）
WHEEL_LEN = 0.46 # 轮子画出来的长度（垂直于滚动方向）
ARROW_SIGN = -1  # 橙色箭头方向：+1 → t = (-sinθ, cosθ)；-1 → 反向（本次取 -1）
OUT = "omni3_layout"


def vec(deg):
    th = math.radians(deg)
    return math.cos(th), math.sin(th)


def tangent(deg):
    th = math.radians(deg)
    return -math.sin(th) * ARROW_SIGN, math.cos(th) * ARROW_SIGN


fig, ax = plt.subplots(figsize=(7.6, 7.6))

# ---- 车体 ----
ax.add_patch(Circle((0, 0), R, fill=True, facecolor="#f2f4f7", edgecolor="#333333", lw=2.0, zorder=1))
ax.plot(0, 0, "o", ms=4, color="#333333", zorder=4)

# ---- 坐标轴 ----
for (ex, ey), txt, col in [
    ((0, 1.62), "前 +y", "#c0392b"),
    ((1.62, 0), "右 +x", "#1e7b3c"),
    ((0, -1.55), "后 -y", "#9aa0a6"),
    ((-1.55, 0), "左 -x", "#9aa0a6"),
]:
    ax.annotate("", xy=(ex, ey), xytext=(0, 0), arrowprops=dict(arrowstyle="-|>", lw=1.8, color=col))
    ax.text(ex * 1.08, ey * 1.08, txt, color=col, ha="center", va="center", fontsize=12)

# ---- 前向中线（M1/M2 的角平分线）----
ax.plot([0, 0], [0, 1.75], ls=(0, (6, 4)), lw=1.2, color="#c0392b", alpha=0.5, zorder=2)

# ---- 三个轮子 ----
for name, deg in WHEELS:
    rx, ry = vec(deg)
    tx, ty = tangent(deg)
    cx, cy = R * rx, R * ry

    # 径向虚线（轮心位置）
    ax.plot([0, cx], [0, cy], ls="--", lw=1.0, color="#b0b7bf", zorder=2)

    # 轮子本体：长轴沿切向（= 滚动方向）；从正上方看全向轮就是一条切向线
    hx, hy = tx * WHEEL_LEN / 2, ty * WHEEL_LEN / 2
    ax.plot([cx - hx, cx + hx], [cy - hy, cy + hy], lw=17, color="#2e5c8a",
            solid_capstyle="butt", zorder=3)

    # 辋子轴：沿径向的细虚线（跟轮子垂直）
    ax.plot([cx - rx * 0.07, cx + rx * 0.07], [cy - ry * 0.07, cy + ry * 0.07],
            lw=2, ls=(0, (2, 2)), color="#dce6f1", zorder=4)

    # 正转驱动方向箭头（沿切向延伸，与轮子同一条线）
    ax.annotate("", xy=(cx + tx * 0.66, cy + ty * 0.66), xytext=(cx + tx * 0.26, cy + ty * 0.26),
                arrowprops=dict(arrowstyle="-|>", lw=2.4, color="#e67e22"), zorder=5)

    # 编号与角度
    ax.text(cx + rx * 0.46, cy + ry * 0.46, f"{name}\nθ={deg:.0f}°",
            ha="center", va="center", fontsize=13, fontweight="bold", color="#1a3d63", zorder=6)

# ---- 图例与说明 ----
ax.set_title("三全向轮底盘布局（俯视，前向 = M1/M2 中线）", fontsize=15, pad=14)

lines = ["橙色箭头 = 该轮「正转」时底盘前进的方向（轮子滚动方向，沿切向）",
         "轮速 = 底盘速度在箭头方向的投影（俯视，θ 从 +x 逆时针）："]
for name, deg in WHEELS:
    a = ARROW_SIGN * -math.sin(math.radians(deg))
    b = ARROW_SIGN * math.cos(math.radians(deg))
    lines.append(f"    v_{name[1:]} = {a:+.3f}·v_x {b:+.3f}·v_y")
lines.append("再叠加自转项：v_i += R·ω（ω 正方向待定）")

ax.text(0, -1.95, "\n".join(lines), ha="center", va="top", fontsize=11.5,
        color="#333333", linespacing=1.7)

ax.set_xlim(-1.9, 1.9)
ax.set_ylim(-2.75, 1.9)
ax.set_aspect("equal")
ax.axis("off")
fig.tight_layout()

for ext in ("png", "svg"):
    fig.savefig(f"{OUT}.{ext}", dpi=220, facecolor="white")
print(f"saved {OUT}.png / {OUT}.svg")
