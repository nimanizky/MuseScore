import tempfile
from pathlib import Path
import unittest

from import_kepatihan_pitch_targets import filename_target, read_map


class PitchTargetsTests(unittest.TestCase):
    def test_source_shift_is_not_target_deviation(self):
        self.assertEqual(filename_target("02-02_F2_-660c__from_Rebab-ord-C3-mp.wav"), (41, 40))
        self.assertEqual(filename_target("03-03_F#2_-566c__from_Rebab-ord-C3-mp.wav"), (42, 34))

    def test_nearest_pitch_and_high_register_source(self):
        self.assertEqual(filename_target("09-02_G#3_56c__from_Rebab-ord-G#3-mp.wav"), (57, -44))
        self.assertEqual(filename_target("12-05_C#4_-73c__from_Rebab-ord-C#4-mp.wav"), (60, 27))
        self.assertEqual(filename_target("21-01_A5_202c__from_Rebab-ord-G5-mp.wav"), (81, 2))

    def test_plain_pitch_name_is_not_zero_cents(self):
        self.assertIsNone(filename_target("Rebab-ord-F3-mp.wav"))

    def test_velocity_dependent_pitch_stays_unknown(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            samples = ["01-01_D2_-1000c__from_Rebab-ord-C3-mp.wav",
                       "02-02_F2_-660c__from_Rebab-ord-C3-mp.wav"]
            for sample in samples:
                (root / sample).touch()
            path = root / "AutoMap_D3_chroma.sfz"
            path.write_text("\n".join(f"<region> sample={s} key=48 tune=0" for s in samples))
            self.assertEqual(read_map(path, []), [])


if __name__ == "__main__":
    unittest.main()
