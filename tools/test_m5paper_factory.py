"""Check factory generation uses the platform launcher and verifies its output."""
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import m5paper_factory as factory


class FakeEnv(dict):
    def subst(self, value):
        return self.get(value, value)


class M5PaperFactoryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.folder = Path(self.temporary.name) / 'path with spaces'
        self.folder.mkdir()
        self.tool = self.folder / 'esptool'
        self.tool.write_bytes(b'launcher')
        self.app = self.folder / 'firmware.bin'
        self.app.write_bytes(b'application')
        self.extra = []
        for offset, name in ((0x1000, 'boot'), (0x8000, 'table'), (0xe000, 'ota')):
            path = self.folder / (name + '.bin')
            path.write_bytes(name.encode())
            self.extra.append((offset, path))
        self.env = FakeEnv({
            '$ERASETOOL': '"' + str(self.tool) + '"',
            '$BUILD_DIR/${PROGNAME}.bin': str(self.app),
            '$ESP32_APP_OFFSET': '0x10000',
            'FLASH_EXTRA_IMAGES': self.extra,
        })

    def merge(self, command, check):
        self.assertTrue(check)
        self.assertEqual(command[0], str(self.tool))
        self.assertEqual(command[1:4], ['--chip', 'esp32', 'merge-bin'])
        combined = bytearray(b'\xff' * (0x10000 + self.app.stat().st_size))
        for offset, path in self.extra + [(0x10000, self.app)]:
            self.assertIn(hex(offset), command)
            self.assertIn(str(path), command)
            data = path.read_bytes()
            combined[offset:offset + len(data)] = data
        output = Path(command[command.index('--output') + 1])
        output.write_bytes(combined)

    def test_uses_configured_launcher_with_spaces_and_validates_app(self):
        # No PioPlatform/get_package_dir method exists on this environment.
        with mock.patch.object(factory.subprocess, 'run', side_effect=self.merge):
            factory.make_factory([], [], self.env)
        self.assertEqual((self.folder / 'firmware.factory.bin').read_bytes()[0x10000:],
                         self.app.read_bytes())

    def test_missing_launcher_is_rejected(self):
        for value in ('', str(self.folder / 'missing-tool')):
            with self.subTest(value=value), self.assertRaisesRegex(RuntimeError, 'launcher'):
                factory.configured_esptool(FakeEnv({'$ERASETOOL': value}))

    def test_unexpected_flash_layout_is_rejected_before_merge(self):
        self.env['FLASH_EXTRA_IMAGES'] = self.extra[:-1]
        with mock.patch.object(factory.subprocess, 'run') as runner:
            with self.assertRaisesRegex(RuntimeError, 'layout changed'):
                factory.make_factory([], [], self.env)
            runner.assert_not_called()

    def test_bad_merged_application_is_rejected(self):
        def corrupt(command, check):
            self.merge(command, check)
            output = Path(command[command.index('--output') + 1])
            output.write_bytes(output.read_bytes()[:-1] + b'!')
        with mock.patch.object(factory.subprocess, 'run', side_effect=corrupt):
            with self.assertRaisesRegex(RuntimeError, 'application region differs'):
                factory.make_factory([], [], self.env)


if __name__ == '__main__':
    unittest.main()
