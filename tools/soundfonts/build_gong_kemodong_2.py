#!/usr/bin/env python3
"""Build the six-key original Gong Kemodong 2 SoundFont from supplied WAVs.

Source WAVs are never modified. FFmpeg converts recordings to dithered 16-bit
PCM at their original sample rate for the existing SF2 writer. Samples are
exact-key stereo zones without transposition or looping.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from sfz_to_sf2 import Region, make_sf2

# Filename, MIDI trigger, Western translation cents (PDF reference only).
SAMPLES = (
    ("Sangat Rendah 3.wav", 41, 16),
    ("Sangat Rendah 5.wav", 44, 45),
    ("Sangat Rendah 6.wav", 47, 38),
    ("Sangat Rendah 7.wav", 48, -32),
    ("Rendah 1.wav", 50, 14),
    ("Rendah 2.wav", 52, -18),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("samples", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    for filename, _, _ in SAMPLES:
        if not (args.samples / filename).is_file():
            raise FileNotFoundError(args.samples / filename)
    with tempfile.TemporaryDirectory(prefix="gong-kemodong-2-") as temp:
        regions = []
        for filename, key, _ in SAMPLES:
            converted = Path(temp) / f"{key}.wav"
            subprocess.run([
                "ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "error",
                "-i", str(args.samples / filename), "-map", "0:a:0",
                "-af", "aresample=osf=s16:dither_method=triangular",
                "-c:a", "pcm_s16le", str(converted),
            ], check=True)
            regions.append(Region(converted, key, key, key, key))
        data = make_sf2(regions, {"ampeg_release": 5.0}, "Gong Kemodong 2")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Wrote {args.output}: {len(regions)} exact keys, {len(data):,} bytes")


if __name__ == "__main__":
    main()
