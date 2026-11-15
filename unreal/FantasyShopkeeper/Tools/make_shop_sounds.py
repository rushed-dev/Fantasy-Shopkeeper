"""Author short shop cues with standard-library additive synthesis.

Sources are original mathematical waveforms. The coin cue develops the damped
1100 Hz coin tone already authored in scripts/checkout_panel.gd.
"""

import json
import math
from pathlib import Path
import random
import struct
import wave

output = Path(__file__).resolve().parents[3] / "art/shop-audio"
output.mkdir(parents=True, exist_ok=True)
sample_rate = 48000
random_source = random.Random(741)
manifest = {}


def write_sound(name, duration, sample):
    values = []
    for frame in range(round(duration * sample_rate)):
        time = frame / sample_rate
        # A short onset and final fade prevent discontinuity clicks.
        fade = min(1, time / 0.002, (duration - time) / 0.025)
        values.append(sample(time) * fade)
    peak = max(abs(value) for value in values)
    assert peak > 0
    pcm = [round(value / peak * 0.65 * 32767) for value in values]
    assert max(abs(value) for value in pcm) < 32767
    with wave.open(str(output / f"{name}.wav"), "wb") as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(sample_rate)
        sound.writeframes(b"".join(struct.pack("<h", value) for value in pcm))
    manifest[name] = dict(duration_seconds=duration, sample_rate=sample_rate,
                          channels=1, bits_per_sample=16, peak=0.65)


def ring(time, frequency, decay):
    return math.sin(math.tau * frequency * time) * math.exp(-time * decay)


write_sound("S_BottlePickup", 0.22, lambda t: ring(t, 860, 25) + 0.3 * ring(t, 1425, 38))
write_sound("S_BottlePlace", 0.25, lambda t: ring(t, 620, 22) + 0.25 * ring(t, 1310, 35)
            + random_source.uniform(-0.25, 0.25) * math.exp(-t * 90))
write_sound("S_CoinTray", 0.14, lambda t: ring(t, 1100, 70) + 0.35 * ring(t, 2420, 95)
            + 0.08 * ring(t, 3530, 100))


def brew(time):
    value = 0
    for onset, frequency in ((0, 180), (0.13, 240), (0.28, 200), (0.43, 310)):
        age = time - onset
        if age >= 0:
            # A descending, damped chirp suggests a small bubble breaking.
            value += math.sin(math.tau * (frequency * age - 90 * age * age)) * math.exp(-age * 18)
    if time >= 0.52:
        value += 0.4 * ring(time - 0.52, 1046.5, 14)
    return value


write_sound("S_BrewComplete", 0.9, brew)
write_sound("S_SaleComplete", 0.55, lambda t: 0.5 * ring(t, 659.25, 12)
            + (0.5 * ring(t - 0.09, 987.77, 11) if t >= 0.09 else 0))
(output / "manifest.json").write_text(json.dumps(manifest, indent=2))
print(f"SHOP_SOUNDS_COMPLETE cues={len(manifest)}")
