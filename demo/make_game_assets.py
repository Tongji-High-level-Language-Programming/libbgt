#!/usr/bin/env python3
"""生成 demo/03-05 游戏资产（零版权：全部用代码合成）。

用法：python demo/make_game_assets.py
依赖：仅 Python 标准库（wave、math、random、struct、zlib）。
WAV 与 examples/make_sound_assets.py 同法合成（固定种子、可再生成、字节确定）；
PNG 用纯标准库手写编码器：字符画 → 调色板 → zlib → IHDR/IDAT/IEND + CRC。
"""

import math
import random
import struct
import wave
import zlib
from pathlib import Path

OUT = Path(__file__).resolve().parent


def write_wav(name, samples, sample_rate):
    path = OUT / name
    with wave.open(str(path), "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(sample_rate)
        frames = bytearray()
        for s in samples:
            value = max(-1.0, min(1.0, s))
            frames += struct.pack("<h", int(value * 32767))
        f.writeframes(bytes(frames))
    print(f"wrote {path} ({len(samples) / sample_rate:.2f}s)")


# ---------- 03_breakout ----------

def breakout_bounce_sound():
    """挡板/墙壁反弹短音：330Hz 纯音 0.06 秒，线性淡出。"""
    rate = 44100
    duration = 0.06
    n = int(duration * rate)
    samples = []
    for i in range(n):
        t = i / rate
        fade = 1.0 - t / duration
        samples.append(0.45 * fade * math.sin(2 * math.pi * 330.0 * t))
    return samples, rate


def breakout_brick_sound():
    """消砖清脆双击：880Hz 与 1174.66Hz 各 0.04 秒，线性淡出。"""
    rate = 44100
    samples = []
    for freq in (880.0, 1174.66):
        duration = 0.04
        n = int(duration * rate)
        for i in range(n):
            t = i / rate
            fade = 1.0 - t / duration
            samples.append(0.4 * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def breakout_lose_sound():
    """失球下滑音：400Hz 滑到 120Hz，0.4 秒线性淡出。"""
    rate = 44100
    duration = 0.4
    n = int(duration * rate)
    samples = []
    for i in range(n):
        t = i / rate
        freq = 400.0 - 280.0 * (i / n)
        fade = 1.0 - t / duration
        samples.append(0.5 * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def breakout_win_sound():
    """过关上扬琶音：C4-E4-G4-C5 各 0.12 秒，平方淡出。"""
    rate = 44100
    samples = []
    for freq in (523.25, 659.26, 783.99, 1046.50):
        duration = 0.12
        n = int(duration * rate)
        for i in range(n):
            t = i / rate
            fade = 1.0 - t / duration
            samples.append(0.45 * fade * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


# ---------- 04_minesweeper ----------

def minesweeper_mine_sound():
    """踩雷爆炸：白噪声叠加 50Hz 低频轰鸣，0.5 秒指数衰减。"""
    rate = 44100
    duration = 0.5
    n = int(duration * rate)
    random.seed(20260907)
    samples = []
    for i in range(n):
        t = i / rate
        decay = math.exp(-6.0 * t)
        noise = random.uniform(-1.0, 1.0)
        rumble = math.sin(2 * math.pi * 50.0 * t)
        samples.append(0.65 * decay * (0.7 * noise + 0.3 * rumble))
    return samples, rate


def minesweeper_win_sound():
    """扫雷胜利小号：G3-C4-E4-G4 上行，每音 0.2 秒，八度泛音加亮。"""
    rate = 44100
    samples = []
    for freq in (392.00, 523.25, 659.26, 783.99):
        duration = 0.2
        n = int(duration * rate)
        for i in range(n):
            t = i / rate
            fade = 1.0 - t / duration
            value = 0.4 * fade * math.sin(2 * math.pi * freq * t)
            value += 0.1 * fade * math.sin(4 * math.pi * freq * t)
            samples.append(value)
    return samples, rate


# ---------- 05_shooter ----------

def shooter_shoot_sound():
    """激光下扫音：1200Hz 滑到 200Hz，0.12 秒线性淡出。"""
    rate = 44100
    duration = 0.12
    n = int(duration * rate)
    samples = []
    for i in range(n):
        t = i / rate
        freq = 1200.0 - 1000.0 * (i / n)
        fade = 1.0 - t / duration
        samples.append(0.35 * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def shooter_boom_sound():
    """敌机爆炸：白噪声 0.4 秒指数衰减（与踩雷音不同种子、不同衰减率）。"""
    rate = 44100
    duration = 0.4
    n = int(duration * rate)
    random.seed(20260908)
    samples = []
    for i in range(n):
        t = i / rate
        decay = math.exp(-9.0 * t)
        samples.append(0.6 * decay * random.uniform(-1.0, 1.0))
    return samples, rate


def shooter_lose_sound():
    """飞船被击毁低鸣：250Hz 滑到 60Hz，0.6 秒线性淡出。"""
    rate = 44100
    duration = 0.6
    n = int(duration * rate)
    samples = []
    for i in range(n):
        t = i / rate
        freq = 250.0 - 190.0 * (i / n)
        fade = 1.0 - t / duration
        samples.append(0.55 * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def shooter_bgm_sound():
    """循环旋律：8 秒、22050Hz 单声道（控制仓库体积）。
    A 小调太空氛围小曲，每音轻微淡入淡出，循环衔接不突兀。"""
    rate = 22050
    notes = [220.00, 261.63, 293.66, 329.63, 293.66, 261.63,
             220.00, 0.0, 246.94, 293.66, 329.63, 369.99,
             329.63, 293.66, 246.94, 0.0]
    beat = 0.5
    samples = []
    for freq in notes:
        n = int(beat * rate)
        for i in range(n):
            t = i / rate
            edge = min(1.0, t / 0.02, (beat - t) / 0.02)
            if freq == 0.0:
                samples.append(0.0)
            else:
                value = 0.35 * edge * math.sin(2 * math.pi * freq * t)
                value += 0.07 * edge * math.sin(4 * math.pi * freq * t)
                samples.append(value)
    return samples, rate


# ---------- PNG：纯标准库编码器（32 位 RGBA，滤镜 0，无隔行） ----------

PALETTE = {
    ".": (0, 0, 0, 0),           # 透明
    "W": (245, 245, 250, 255),   # 白（船体/机翼）
    "G": (170, 170, 185, 255),   # 灰（船身）
    "B": (90, 160, 255, 255),    # 蓝（驾驶舱）
    "O": (255, 140, 30, 255),    # 橙（引擎火焰）
    "D": (70, 200, 90, 255),     # 绿（敌人主体）
    "M": (255, 90, 255, 255),    # 洋红（敌人眼睛）
}


def mirror(rows):
    """左半字符画镜像成整图：每行 16 字符 → 32 字符。"""
    return [row + row[::-1] for row in rows]


def write_png(name, rows):
    """把字符画写成 32 位 RGBA PNG（IHDR/IDAT/IEND + CRC）。"""
    width = len(rows[0])
    height = len(rows)
    raw = bytearray()
    for row in rows:
        raw.append(0)  # 滤镜类型 0（None），每行一个字节
        for ch in row:
            raw += bytes(PALETTE[ch])

    def chunk(tag, data):
        block = tag + data
        crc = zlib.crc32(block) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + block + struct.pack(">I", crc)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)  # 8 位 RGBA
    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + chunk(b"IEND", b""))
    path = OUT / name
    path.write_bytes(data)
    print(f"wrote {path} ({width}x{height})")


# 飞船：白色尖头 nose、灰色船身、蓝色驾驶舱、白色后掠机翼、橙色引擎焰。
# 只画左半边（16 列），mirror() 补出右半边。
SHIP_LEFT = [
    "................",
    "................",
    "...............W",
    "...............W",
    "..............WW",
    "..............WW",
    "..............WG",
    "..............WG",
    ".............WWG",
    ".............WWG",
    ".............WGB",
    ".............WGB",
    "............WWGB",
    "............WWGB",
    "...........WWWGB",
    "...........WWWGB",
    "..........WWWWGB",
    "..........WWWWGB",
    ".........WWWWWGB",
    ".........WWWWWGB",
    "........WWWWWWGG",
    "........WWWWWWGG",
    ".........WWWWGGG",
    ".........WWWWGGG",
    "..........WWGGGG",
    "..........WGGGGG",
    "..........WGGGOO",
    "..........WGGGOO",
    "...........WGOOO",
    "...........WGOOO",
    "............WOOO",
    "................",
]

# 敌人：绿色太空侵略者——触角、加宽的头、洋红大眼、体侧螯肢、三条腿。
ENEMY_LEFT = [
    "................",
    "................",
    "................",
    "................",
    ".....D..........",
    ".....D..........",
    "......DD........",
    "......DDD.......",
    ".......DDD......",
    ".....DDDDDD.....",
    "....DDDDDDDDD...",
    "...DDDDDDDDDDDD.",
    "..DDDDDDDDDDDDDD",
    "..DDDDDDDDDDDDDD",
    ".DDDDDDDDDDDDDDD",
    ".DDD.MMMMMDDDDDD",
    ".DDD.MMMMMDDDDDD",
    ".DDDDDDDDDDDDDDD",
    "..DDDDDDDDDDDDDD",
    "..DDDDDDDDDDDDDD",
    "...DDDDDDDDDDDD.",
    "....DDDDDDDDDD..",
    "....DDDD..DDDDDD",
    "....DDD...DDDDDD",
    ".....DD....DDDDD",
    ".....D.....DDDDD",
    "..........DDDDDD",
    "..........DDDDDD",
    "...........DDDDD",
    "...........DDDDD",
    "...........DD.DD",
    "...........DD.DD",
]


def main():
    write_wav("03_bounce.wav", *breakout_bounce_sound())
    write_wav("03_brick.wav", *breakout_brick_sound())
    write_wav("03_lose.wav", *breakout_lose_sound())
    write_wav("03_win.wav", *breakout_win_sound())
    write_wav("04_mine.wav", *minesweeper_mine_sound())
    write_wav("04_win.wav", *minesweeper_win_sound())
    write_wav("05_shoot.wav", *shooter_shoot_sound())
    write_wav("05_boom.wav", *shooter_boom_sound())
    write_wav("05_lose.wav", *shooter_lose_sound())
    write_wav("05_bgm.wav", *shooter_bgm_sound())
    write_png("05_ship.png", mirror(SHIP_LEFT))
    write_png("05_enemy.png", mirror(ENEMY_LEFT))


if __name__ == "__main__":
    main()
