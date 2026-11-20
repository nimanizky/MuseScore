#!/usr/bin/env python3
"""Build the six-key original Gong Kemodong 1 SoundFont from supplied WAVs.

Source WAVs are never modified. FFmpeg converts 24-bit/float recordings to
dithered 16-bit PCM at their original sample rate for the existing SF2 writer.
Samples are exact-key stereo zones without transposition or looping.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from sfz_to_sf2 import Region, make_sf2

# Filename, MIDI trigger, Western translation cents (PDF reference only).
SAMPLES = (
    ("GK 1 - Very Low 3.wav", 42, 11),
    ("GK 1 - Very Low 5.wav", 45, -13),
    ("GK 1 - Very Low 6.wav", 46, -16),
    ("GK 1 - Very Low 7.wav", 49, 6),
    ("GK 1 - Low 1.wav", 50, 0),
    ("GK 1 - Low 2.wav", 51, -10),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("samples", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    for filename, _, _ in SAMPLES:
        if not (args.samples / filename).is_file():
            raise FileNotFoundError(args.samples / filename)
    with tempfile.TemporaryDirectory(prefix="gong-kemodong-1-") as temp:
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
        data = make_sf2(regions, {"ampeg_release": 5.0}, "Gong Kemodong 1")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Wrote {args.output}: {len(regions)} exact keys, {len(data):,} bytes")


if __name__ == "__main__":
    main()
