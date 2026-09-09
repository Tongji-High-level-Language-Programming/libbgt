#!/usr/bin/env python3
"""生成 examples/13_*.wav 声音资产（零版权：全部用代码合成）。

用法：python examples/make_sound_assets.py
依赖：仅 Python 标准库（wave、math、random、struct）。
"""

import math
import random
import struct
import wave
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


def jump_sound():
    """上扬短音：0.3 秒内频率从 300Hz 滑到 900Hz，音量线性淡出。"""
    rate = 44100
    duration = 0.3
    n = int(duration * rate)
    samples = []
    for i in range(n):
        t = i / rate
        freq = 300.0 + 600.0 * (i / n)
        fade = 1.0 - t / duration
        samples.append(0.5 * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def ding_sound():
    """叮咚双音：523Hz（do）与 784Hz（sol）各 0.25 秒，平方淡出。"""
    rate = 44100
    samples = []
    for freq in (523.25, 783.99):
        duration = 0.25
        n = int(duration * rate)
        for i in range(n):
            t = i / rate
            fade = 1.0 - t / duration
            samples.append(0.45 * fade * fade * math.sin(2 * math.pi * freq * t))
    return samples, rate


def boom_sound():
    """爆炸：白噪声叠加 60Hz 低频轰鸣，0.4 秒指数衰减。"""
    rate = 44100
    duration = 0.4
    n = int(duration * rate)
    random.seed(20260905)
    samples = []
    for i in range(n):
        t = i / rate
        decay = math.exp(-8.0 * t)
        noise = random.uniform(-1.0, 1.0)
        rumble = math.sin(2 * math.pi * 60.0 * t)
        samples.append(0.6 * decay * (0.7 * noise + 0.3 * rumble))
    return samples, rate


def melody_sound():
    """循环旋律：8 秒、22050Hz 单声道（控制仓库体积）。
    C 大调五声音阶小曲，每音轻微淡入淡出，循环衔接不突兀。"""
    rate = 22050
    notes = [261.63, 293.66, 329.63, 392.00, 440.00,
             392.00, 329.63, 293.66, 261.63, 0.0,
             293.66, 329.63, 392.00, 440.00, 523.25, 0.0]
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
                value = 0.4 * edge * math.sin(2 * math.pi * freq * t)
                value += 0.08 * edge * math.sin(4 * math.pi * freq * t)
                samples.append(value)
    return samples, rate


def main():
    write_wav("11_jump.wav", *jump_sound())
    write_wav("11_ding.wav", *ding_sound())
    write_wav("11_boom.wav", *boom_sound())
    write_wav("11_melody.wav", *melody_sound())


if __name__ == "__main__":
    main()
