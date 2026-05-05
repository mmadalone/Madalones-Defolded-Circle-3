#!/usr/bin/env python3
"""Generate CC0 substitute sound effects for UC remote-ui.

Replaces UC's stock wav files when UC_SOUND_EFFECTS_PATH is empty (firmware 2.9.2 regression).
Outputs to deploy/config/:

  click.wav       - short crisp click (UI button feedback, 60 ms, 1500 Hz)
  click_lo.wav    - same shape, lower pitch (alt feedback, 60 ms, 900 Hz)
  confirm.wav     - rising tone pair (action confirmed)
  error.wav       - descending warning tone (action failed)

  zap_future.wav   - dock chime variant 1: descending sweep + harmonic (sci-fi/warp, default)
  zap_arpeggio.wav - dock chime variant 2: 4-note ascending arpeggio (welcoming)
  zap_bell.wav     - dock chime variant 3: single bell tone (classic)
  zap_dyad.wav     - dock chime variant 4: two-tone harmonic dyad (chord)
  zap_synthwave.wav - dock chime variant 5: AM-modulated retro pulse (synthwave)
  zap_strike.wav   - dock chime variant 6: rapid sawtooth pitch-plummet + crackle (electric zap)

Format: 44.1 kHz, 16-bit PCM, **stereo** (UCR3's ALSA backend rejects mono with
snd_pcm_hw_params_set_channels: err = -22).
License: CC0 1.0 Universal — public domain, no attribution required.
"""
import wave
from pathlib import Path

import numpy as np

SAMPLE_RATE = 44100
OUT_DIR = Path(__file__).resolve().parents[1] / "deploy" / "config"


def envelope_attack_release(n_samples, attack_ms, release_ms, peak=1.0):
    """Linear attack + exponential release envelope."""
    attack_n = int(SAMPLE_RATE * attack_ms / 1000)
    release_n = max(1, int(SAMPLE_RATE * release_ms / 1000))
    env = np.zeros(n_samples)
    if attack_n > 0:
        env[:attack_n] = np.linspace(0, peak, attack_n)
    if release_n > 0:
        decay = np.exp(-np.linspace(0, 5, release_n))
        env[attack_n:attack_n + release_n] = peak * decay[:n_samples - attack_n]
    return env


def write_wav(path: Path, samples: np.ndarray, target_peak: float = 1.0,
              soft_drive: float = 2.2, pre_gain: float = 1.2):
    """Write float samples as 16-bit PCM stereo wav, with loudness boost.

    Three-stage loudness chain (gentle saturation, no brick-wall):
    1. Pre-gain (1.2x) — modest signal lift before the saturator.
    2. Soft-clip via tanh(samples * soft_drive) / tanh(soft_drive) — at drive=2.2 this
       gives a ~6-8 dB perceived RMS boost without aggressive harmonic distortion.
       Earlier iteration used drive=3.0 which sounded choppy on the chime tails (long
       exponential decays got squashed flat against the brick-wall ceiling); 2.2 keeps
       the tails breathing.
    3. Peak-normalize to `target_peak` (1.0 = full int16 range) so every wav uses the
       full dynamic range regardless of synthesis-pipeline amplitude summing.
    4. Final clip to [-1, 1] is defensive; prior steps should keep us in range.
    5. Mono signal duplicated to L+R (UCR3's ALSA backend rejects mono with EINVAL -22).
    """
    if pre_gain != 1.0:
        samples = samples * pre_gain
    if soft_drive > 1.0:
        samples = np.tanh(samples * soft_drive) / np.tanh(soft_drive)
    peak = float(np.max(np.abs(samples)))
    if peak > 0:
        samples = samples * (target_peak / peak)
    samples = np.clip(samples, -1.0, 1.0)
    # Use 32766 instead of 32767 to avoid the lone +32768 trap when peak=1.0 and any
    # rounding tips a sample past int16 max (would wrap to -32768 = pop).
    pcm = (samples * 32766).astype(np.int16)
    stereo = np.empty(pcm.size * 2, dtype=np.int16)
    stereo[0::2] = pcm
    stereo[1::2] = pcm
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(stereo.tobytes())
    print(f"  wrote {path.name:22s} {len(pcm):6d} frames ({len(pcm)/SAMPLE_RATE*1000:.0f} ms, peak={peak:.2f} -> {target_peak:.2f}, {path.stat().st_size} bytes)")


# -----------------------------------------------------------------------------
# Click family — louder, longer, lower-frequency than first iteration. UCR3's
# tiny speaker has poor response above ~2 kHz, and short bursts (<40 ms) get
# eaten by ALSA buffer latency on small embedded sinks.
# -----------------------------------------------------------------------------
def gen_click(freq_hz=1500.0, duration_ms=60):
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    # Tonal body + tiny pink-ish noise burst at the very start for "click" texture.
    tone = np.sin(2 * np.pi * freq_hz * t)
    rng = np.random.default_rng(42)
    noise = rng.standard_normal(n) * 0.3 * np.exp(-t * 200)
    body = 0.85 * tone + 0.15 * noise
    env = envelope_attack_release(n, attack_ms=2, release_ms=duration_ms - 2, peak=0.9)
    return body * env


def gen_confirm():
    """Two-note rising chime: 880 Hz then 1320 Hz. ~180 ms."""
    note_ms = 90
    n_per = int(SAMPLE_RATE * note_ms / 1000)
    out = np.zeros(2 * n_per)
    for i, freq in enumerate((880.0, 1320.0)):
        t = np.arange(n_per) / SAMPLE_RATE
        tone = np.sin(2 * np.pi * freq * t)
        env = envelope_attack_release(n_per, attack_ms=3, release_ms=note_ms - 3, peak=0.7)
        out[i * n_per:(i + 1) * n_per] = tone * env
    return out


def gen_error():
    """Two-note descending: 660 Hz then 440 Hz, slight detune for buzzy feel. ~220 ms."""
    note_ms = 110
    n_per = int(SAMPLE_RATE * note_ms / 1000)
    out = np.zeros(2 * n_per)
    for i, freq in enumerate((660.0, 440.0)):
        t = np.arange(n_per) / SAMPLE_RATE
        tone = 0.7 * np.sin(2 * np.pi * freq * t) + 0.3 * np.sin(2 * np.pi * freq * 1.005 * t)
        env = envelope_attack_release(n_per, attack_ms=4, release_ms=note_ms - 4, peak=0.7)
        out[i * n_per:(i + 1) * n_per] = tone * env
    return out


# -----------------------------------------------------------------------------
# Dock chimes — 5 distinct variants. User picks via Settings → Sound dropdown
# (Config.dockChimeVariant 1..5). Variant 1 (Warp) is loaded from zap_future.wav
# for stock-firmware filename compatibility; variants 2..5 use descriptive names.
# -----------------------------------------------------------------------------
def chime_warp():
    """Variant 1 (default): sci-fi descending sweep 1800 → 600 Hz + harmonic + shimmer. ~340 ms."""
    duration_ms = 340
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    freq = np.linspace(1800.0, 600.0, n)
    phase = 2 * np.pi * np.cumsum(freq) / SAMPLE_RATE
    sweep = np.sin(phase)
    harmonic = 0.4 * np.sin(2 * np.pi * 880.0 * t)
    shimmer = 0.2 * np.sin(2 * np.pi * 3520.0 * t) * np.exp(-t * 8)
    body = 0.55 * sweep + 0.35 * harmonic + 0.10 * shimmer
    env = envelope_attack_release(n, attack_ms=8, release_ms=duration_ms - 8, peak=0.7)
    return body * env


def chime_arpeggio():
    """Variant 2: 4-note ascending arpeggio (C-E-G-C). ~360 ms total, 90 ms per note."""
    notes_hz = (523.25, 659.25, 783.99, 1046.50)  # C5 E5 G5 C6
    note_ms = 90
    n_per = int(SAMPLE_RATE * note_ms / 1000)
    out = np.zeros(len(notes_hz) * n_per)
    for i, freq in enumerate(notes_hz):
        t = np.arange(n_per) / SAMPLE_RATE
        tone = np.sin(2 * np.pi * freq * t) + 0.3 * np.sin(2 * np.pi * freq * 2 * t)
        env = envelope_attack_release(n_per, attack_ms=4, release_ms=note_ms - 4, peak=0.65)
        out[i * n_per:(i + 1) * n_per] = tone * env
    return out


def chime_bell():
    """Variant 3: single mellow bell tone at 880 Hz with long exponential decay. ~500 ms."""
    duration_ms = 500
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    fundamental = np.sin(2 * np.pi * 880.0 * t)
    overtone1 = 0.3 * np.sin(2 * np.pi * 1760.0 * t)  # octave
    overtone2 = 0.15 * np.sin(2 * np.pi * 2640.0 * t)  # octave + fifth
    inharmonic = 0.1 * np.sin(2 * np.pi * 2950.0 * t)  # bell-like inharmonicity
    body = fundamental + overtone1 + overtone2 + inharmonic
    # Slow exponential decay for a true bell shape.
    env = envelope_attack_release(n, attack_ms=2, release_ms=duration_ms - 2, peak=0.7)
    return body * env


def chime_dyad():
    """Variant 4: two-tone harmonic dyad held together (perfect fifth). ~280 ms."""
    duration_ms = 280
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    # Perfect fifth: A4 (440) and E5 (659.25) with shared envelope.
    tone_a = np.sin(2 * np.pi * 440.0 * t) + 0.4 * np.sin(2 * np.pi * 880.0 * t)
    tone_b = np.sin(2 * np.pi * 659.25 * t) + 0.3 * np.sin(2 * np.pi * 1318.5 * t)
    body = 0.5 * tone_a + 0.5 * tone_b
    env = envelope_attack_release(n, attack_ms=10, release_ms=duration_ms - 10, peak=0.65)
    return body * env


def chime_synthwave():
    """Variant 5: AM-modulated retro pulse — 6 Hz tremolo on a 660 Hz carrier. ~400 ms."""
    duration_ms = 400
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    carrier = np.sin(2 * np.pi * 660.0 * t)
    sub = 0.3 * np.sin(2 * np.pi * 330.0 * t)  # sub-octave for body
    # 6 Hz tremolo modulation, depth 0.5.
    am = 0.5 + 0.5 * np.sin(2 * np.pi * 6.0 * t)
    body = (carrier + sub) * am
    env = envelope_attack_release(n, attack_ms=10, release_ms=duration_ms - 10, peak=0.65)
    return body * env


def chime_strike():
    """Variant 6 (Zap): rapid sawtooth pitch-plummet 4000 → 200 Hz + crackle + low rumble. ~250 ms.

    Punchy electric character — sawtooth wave for buzziness, fast pitch sweep for the "ZAP!"
    plummet, exponentially-decaying noise crackle for grit, low rumble for body. Distinct from
    the smoother `chime_warp()` which uses sine waves and a slower, more melodic sweep.
    """
    duration_ms = 250
    n = int(SAMPLE_RATE * duration_ms / 1000)
    t = np.arange(n) / SAMPLE_RATE
    # Fast pitch plummet 4000 → 200 Hz over the first 100 ms, then sustain.
    sweep_n = int(SAMPLE_RATE * 0.10)
    freq = np.empty(n)
    freq[:sweep_n] = np.linspace(4000.0, 200.0, sweep_n)
    freq[sweep_n:] = 200.0
    phase = 2 * np.pi * np.cumsum(freq) / SAMPLE_RATE
    # Sawtooth wave: 2*(phase/2π - floor(0.5 + phase/2π)) gives [-1, 1] sawtooth.
    saw = 2 * (phase / (2 * np.pi) - np.floor(0.5 + phase / (2 * np.pi)))
    # Crackle that fades fast.
    rng = np.random.default_rng(7)
    crackle = rng.standard_normal(n) * 0.4 * np.exp(-t * 25)
    # Sub-bass rumble for body.
    rumble = 0.3 * np.sin(2 * np.pi * 80.0 * t) * np.exp(-t * 8)
    body = 0.6 * saw + 0.25 * crackle + 0.15 * rumble
    env = envelope_attack_release(n, attack_ms=2, release_ms=duration_ms - 2, peak=0.85)
    return body * env


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f"Writing to {OUT_DIR}")

    # UI feedback sounds
    write_wav(OUT_DIR / "click.wav",    gen_click(freq_hz=1500.0, duration_ms=60))
    write_wav(OUT_DIR / "click_lo.wav", gen_click(freq_hz=900.0,  duration_ms=60))
    write_wav(OUT_DIR / "confirm.wav",  gen_confirm())
    write_wav(OUT_DIR / "error.wav",    gen_error())

    # Dock chime variants — 6 total. Variant 1 is the default (zap_future.wav for
    # stock-firmware filename compatibility); variants 2-6 use descriptive names.
    write_wav(OUT_DIR / "zap_future.wav",     chime_warp())       # variant 1 — Warp
    write_wav(OUT_DIR / "zap_arpeggio.wav",   chime_arpeggio())   # variant 2 — Ascend
    write_wav(OUT_DIR / "zap_bell.wav",       chime_bell())       # variant 3 — Bell
    write_wav(OUT_DIR / "zap_dyad.wav",       chime_dyad())       # variant 4 — Chord
    write_wav(OUT_DIR / "zap_synthwave.wav",  chime_synthwave())  # variant 5 — Pulse
    write_wav(OUT_DIR / "zap_strike.wav",     chime_strike())     # variant 6 — Zap

    print("done.")


if __name__ == "__main__":
    main()
