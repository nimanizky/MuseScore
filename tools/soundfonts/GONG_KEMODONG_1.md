# Gong Kemodong 1 (Original Tuning)

Source: the six user-supplied `GK 1` WAV recordings and
`Tuning Gong Kemodong 1.pdf`. Short score name: `GK 1 Orig.`

| Recording | Kepatihan | Trigger key | Western translation |
| --- | --- | --- | --- |
| GK 1 - Very Low 3.wav | 3, two dots below | 42 | F♯2 +11¢ |
| GK 1 - Very Low 5.wav | 5, two dots below | 45 | A2 −13¢ |
| GK 1 - Very Low 6.wav | 6, two dots below | 46 | B♭2 −16¢ |
| GK 1 - Very Low 7.wav | 7, two dots below | 49 | C♯3 +6¢ |
| GK 1 - Low 1.wav | 1, one dot below | 50 | D3 ±0¢ |
| GK 1 - Low 2.wav | 2, one dot below | 51 | E♭3 −10¢ |

These are the six supported recorded pitches. Degree 4 is absent. The
notation profile uses middle-register proxy pitches D4, E♭4, F♯4, A4, B♭4,
C♯5 to derive dots. Each degree specifies its recorded register, so normal
number entry automatically chooses the matching low or very-low recording.
Explicit octave changes outside that degree's recorded register are rejected. Only the
six exact trigger keys above contain samples; no other notes are synthesized.

The PDF cents are a display reference. Sample playback uses the recording
unchanged in pitch, without applying those cents a second time. Recordings
remain stereo and keep their original sample rate. The existing SF2 writer
requires 16-bit PCM, so the builder dithers higher-depth/float sources to
16-bit in temporary files. Original WAV files are never modified. No sample
loops, trimming, normalization, or pitch shifting are applied.

Build from the source folder:

```sh
python3 tools/soundfonts/build_gong_kemodong_1.py \
  "/path/to/Gong Kemodong 1 - Tuning Sample and Table" \
  soundfont/Gamelan_Gong_Kemodong_1_Original_Tuning.sf2
```

CMake discovers `soundfont/Gamelan_*.sf2` and bundles the generated file.
The binary remains outside Git, following the project's existing sample
asset policy. The notation template, playback setup ID, SoundFont resource
mapping, and Western translation table all use
`gong-kemodong-1-original-tuning`.
