# Gong Kemodong 2 (Original Tuning)

Short score name: `GK 2 Orig.`

The six samples and their original tuning references from
`VERSION 6 APRIL - Tabel Referensi Kemodong 2 untuk Handoko.pdf` are:

| Kepatihan | Octave marks | File | Western pitch | cents | MIDI key |
|---|---:|---|---|---:|---:|
| 3 | two below | `Sangat Rendah 3.wav` | F2 | +16¢ | 41 |
| 5 | two below | `Sangat Rendah 5.wav` | G♯2 | +45¢ | 44 |
| 6 | two below | `Sangat Rendah 6.wav` | B2 | +38¢ | 47 |
| 7 | two below | `Sangat Rendah 7.wav` | C3 | −32¢ | 48 |
| 1 | one below | `Rendah 1.wav` | D3 | +14¢ | 50 |
| 2 | one below | `Rendah 2.wav` | E3 | −18¢ | 52 |

Degree 4 has no supplied recording. Cents are shown in the Western-pitch
message only; the recordings are already tuned and are not retuned during
SoundFont creation.

Build from the supplied source folder with:

```sh
python3 tools/soundfonts/build_gong_kemodong_2.py \
  "/path/to/Gong Kemodong 2 - Tabel dan Referensi Tuning" \
  soundfont/Gamelan_Gong_Kemodong_2_Original_Tuning.sf2
```

The script keeps each recording's sample rate and channel layout, converts to
dithered 16-bit PCM for the SF2 writer, and maps each sample to its exact key.
