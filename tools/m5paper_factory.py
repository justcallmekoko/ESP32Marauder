"""Generate the classic-ESP32 first-install image with `pio run -t factory`.

The pinned 55.03.34 platform does not generate firmware.factory.bin itself.
Use the same extra images as its upload target; never read a connected device.
"""
from pathlib import Path
import subprocess

Import('env')


def make_factory(source, target, env):
    app = Path(env.subst('$BUILD_DIR/${PROGNAME}.bin'))
    output = app.with_name(app.stem + '.factory.bin')
    extra = [(int(str(offset), 0), Path(env.subst(str(path))))
             for offset, path in env.get('FLASH_EXTRA_IMAGES', [])]
    app_offset = int(env.subst('$ESP32_APP_OFFSET'), 0)
    if sorted(offset for offset, _ in extra) != [0x1000, 0x8000, 0xe000]:
        raise RuntimeError('M5Paper classic-ESP32 boot image layout changed')
    if app_offset != 0x10000 or app.stat().st_size > 0x400000:
        raise RuntimeError('M5Paper application does not fit its partition')
    images = sorted(extra + [(app_offset, app)])
    esptool = Path(env.PioPlatform().get_package_dir('tool-esptoolpy')) / 'esptool.py'
    command = [env.subst('$PYTHONEXE'), str(esptool), '--chip', 'esp32',
               'merge-bin', '--output', str(output), '--flash-mode', 'dio',
               '--flash-freq', '40m', '--flash-size', '16MB']
    for offset, path in images:
        if not path.is_file():
            raise RuntimeError(f'Missing flash component: {path}')
        command += [hex(offset), str(path)]
    subprocess.run(command, check=True)
    combined = output.read_bytes()
    if combined[app_offset:] != app.read_bytes():
        raise RuntimeError('Factory image application region differs from firmware.bin')
    print(f'M5Paper factory image verified: {len(combined)} bytes at offset 0x0')


env.AddCustomTarget(
    'factory',
    ['$BUILD_DIR/${PROGNAME}.bin'] +
    [env.subst(str(path)) for _, path in env.get('FLASH_EXTRA_IMAGES', [])],
    make_factory,
    title='M5Paper factory image',
    description='Build and merge the M5Paper v1.1 first-install image at 0x0',
)
