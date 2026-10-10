"""Publish only freshly built outputs; record compiler, macros, source and HEX hashes."""
from pathlib import Path
import hashlib
import json
import shlex
import shutil
import subprocess
from profiles import PROFILES

root = Path(__file__).resolve().parent.parent
release = root / 'releases'
source_paths = sorted([*root.glob('src/*.c'), *root.glob('include/*.h'),
                       *root.glob('tools/*.py'), root / 'Makefile',
                       root / 'studio/MonitorATmega16.cproj'])
sources = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths}
names = {'base': 'BASE_UART_OFF', 'uart': 'BASE_UART_ON', 'full': 'FULL', 'rtc': 'RTC',
         'light': 'LIGHT', 'passive': 'PASSIVE', 'diagnostic': 'DIAGNOSTIC', 'minimal': 'MINIMAL'}
manifest = {'mcu': 'atmega16', 'signature': '0x1e9403', 'f_cpu': 8000000,
            'source_sha256': sources, 'builds': {}}
for legacy in (False, True):
    for profile in PROFILES:
        name = ('legacy-' if legacy else '') + profile
        build = root / 'build' / name
        # .flags is written by the actual Make build, not reconstructed by packaging.
        stamp = (build / '.flags').read_text().splitlines()
        compiler, flags = stamp[:2]
        version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
        if legacy:
            assert '5.4.0' in version
        else:
            assert '14.2.0' in version
        cmd = [compiler] + shlex.split(flags) + ['-dM', '-E', '-x', 'c', 'include/config.h']
        macros = {}
        for line in subprocess.check_output(cmd, cwd=root, text=True).splitlines():
            fields = line.split()
            if len(fields) == 3 and (fields[1].startswith('ENABLE_') or fields[1] in
                                    ('BUZZER_ACTIVE', 'DIAG_LED7', 'BH1750_ADDRESS', 'F_CPU')):
                macros[fields[1]] = fields[2]
        assert macros['ENABLE_UART'] == ('0' if profile in ('base', 'minimal') else '1')
        for source in source_paths:
            if source.suffix in ('.c', '.h'):
                assert (build / 'monitor.elf').stat().st_mtime_ns >= source.stat().st_mtime_ns, str(source)
        dest = release / name
        dest.mkdir(parents=True, exist_ok=True)
        for extension in ('hex', 'elf', 'map', 'eep', 'size.json'):
            shutil.copy2(build / ('monitor.' + extension), dest / ('monitor.' + extension))
        (dest / 'build-flags.txt').write_text('\n'.join(stamp) + '\n')
        info = {'compiler': version, 'flags': flags, 'macros': macros,
                'size': json.loads((dest / 'monitor.size.json').read_text()),
                'hex_sha256': hashlib.sha256((dest / 'monitor.hex').read_bytes()).hexdigest(),
                'elf_sha256': hashlib.sha256((dest / 'monitor.elf').read_bytes()).hexdigest()}
        manifest['builds'][name] = info
        if legacy:
            windows = release / 'windows-gcc54'
            windows.mkdir(exist_ok=True)
            for ext in ('hex', 'elf'):
                shutil.copy2(dest / ('monitor.' + ext), windows / ('ATmega16_DHT11_' + names[profile] + '.' + ext))
(release / 'BUILD_MANIFEST.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n')
files = sorted(p for p in release.rglob('*') if p.is_file() and p.name != 'SHA256SUMS.txt')
(release / 'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest() +
    '  ' + str(p.relative_to(release)) + '\n' for p in files))
print('Packaged 16 rebuilt profiles plus named GCC 5.4 HEX/ELF; source hashes and actual macro flags recorded.')
