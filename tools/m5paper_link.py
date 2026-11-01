"""Board-local equivalent of CONFIG_SPIRAM_CACHE_LIBTIME_IN_IRAM=n.

Arduino ships a prebuilt SDK. Its conservative PSRAM configuration puts libc
calendar/time formatting into scarce classic-ESP32 IRAM. These task-context
functions can use flash, as documented in ESP-IDF's 'Putting C Library in Flash'.
Keep interrupt code, memory primitives, radio code and the memory map unchanged.
Never edit the installed SDK or affect another target.
"""
from pathlib import Path
import re

Import('env')

sdk = Path(env.PioPlatform().get_package_dir('framework-arduinoespressif32-libs'))
source = sdk / 'esp32' / 'ld' / 'sections.ld'
text = source.read_text()
begin = text.index('  .iram0.text :')
end = text.index('  .flash.text :')
names = ('asctime asctime_r ctime ctime_r lcltime lcltime_r gmtime gmtime_r '
         'strftime mktime tzset_r tzset time gettzinfo systimes month_lengths '
         'timelocal tzvars tzlock tzcalc_limits strptime').split()
pattern = (r'^\s*\*libc\.a:libc_a-(?:' + '|'.join(names) +
           r')\.\*\(\.literal \.literal\.\* \.text \.text\.\*\)\n')
rules = re.findall(pattern, text[begin:end], flags=re.MULTILINE)
region, count = re.subn(pattern, '', text[begin:end], flags=re.MULTILINE)
if count != len(names):
    raise RuntimeError(f'M5Paper linker contract changed: {count}/{len(names)} time rules')
patched = text[:begin] + region + text[end:]
flash_marker = '    _text_start = ABSOLUTE(.);\n'
if patched.count(flash_marker) != 1:
    raise RuntimeError('M5Paper flash text section changed')
# The SDK's generic flash rule excludes these libc objects. Place them
# explicitly, avoiding orphan sections and preserving normal image padding.
patched = patched.replace(flash_marker, flash_marker + ''.join(rules), 1)
folder = Path(env.subst('$BUILD_DIR')) / 'm5paper_ld'
folder.mkdir(parents=True, exist_ok=True)
target = folder / 'sections.ld'
if not target.exists() or target.read_text() != patched:
    target.write_text(patched)
env.Prepend(LIBPATH=[str(folder)])
env.Depends('$BUILD_DIR/${PROGNAME}.elf', str(target))
print(f'M5Paper: {count} libc time rules use flash; IRAM primitives retained')
