"""All sketch sources must parse on the legacy Arduino C++11 targets."""
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]


class EpaperModelCompatibilityTests(unittest.TestCase):
    def test_models_compile_as_cpp11_even_without_epaper_target_flags(self):
        # Arduino compiles every sketch .cpp, including these portable models,
        # for boards using the 2.0.11 core's -std=gnu++11 default.
        command = ['g++', '-std=c++11', '-Wall', '-Wextra', '-Werror',
                   '-pedantic-errors', '-fsyntax-only']
        command += [str(ROOT / 'esp32_marauder' / name)
                    for name in ('StickyModel.cpp', 'M5PaperModel.cpp')]
        subprocess.run(command, check=True, capture_output=True, text=True)


if __name__ == '__main__':
    unittest.main()
