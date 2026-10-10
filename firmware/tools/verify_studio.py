"""Compile all XML configurations from Studio's compiler working directories.

This reproduces compiler path resolution on Linux, not the Windows GUI/targets.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import xml.etree.ElementTree as ET
from profiles import STUDIO, definitions

parser = argparse.ArgumentParser()
parser.add_argument('--compiler', default='avr-gcc')
parser.add_argument('--output', default='build/studio-check')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
studio = root / 'studio'
ns = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
project = ET.parse(studio / 'MonitorATmega16.cproj')
assert project.find('.//m:ToolchainName', ns).text == 'com.Atmel.AVRGCC8.C'
items = [studio / x.attrib['Include'].replace('\\', '/')
         for x in project.findall('.//m:Compile', ns)]
assert all(p.is_file() for p in items)
sources = [p.resolve() for p in items if p.suffix == '.c']
assert set(sources) == set(p.resolve() for p in (root / 'src').glob('*.c'))
configs = project.findall('m:PropertyGroup[@Condition]', ns)
assert len(configs) == len(STUDIO)
reports = {}
version = subprocess.check_output([args.compiler, '--version'], text=True).splitlines()[0]
for group in configs:
    name = re.search(r"== '([^']+)'", group.attrib['Condition'])[1]
    output_path = group.find('m:OutputPath', ns).text.replace('\\', '/')
    assert output_path == name + '/'
    cwd = studio / name
    cwd.mkdir(exist_ok=True)
    values = [p.text for p in group.findall('.//m:avrgcc.compiler.symbols.DefSymbols/m:ListValues/m:Value', ns)]
    assert len(values) == len(set(p.split('=')[0] for p in values))
    assert values == ['F_CPU=8000000UL'] + [d[2:] for d in definitions(STUDIO[name])]
    includes = [p.text.replace('\\', '/') for p in group.findall('.//m:avrgcc.compiler.directories.IncludePaths/m:ListValues/m:Value', ns)]
    assert includes == ['../../include']
    assert all((cwd / inc / 'hal.h').is_file() for inc in includes)
    target = (root / args.output / name).resolve()
    target.mkdir(parents=True, exist_ok=True)
    flags = ['-mmcu=atmega16', '-std=gnu99', '-Os', '-g', '-Wall', '-Wextra', '-Werror',
             '-ffunction-sections', '-fdata-sections'] + ['-D' + d for d in values]
    flags += ['-I' + d for d in includes]
    macros_text = subprocess.check_output([args.compiler] + flags +
        ['-dM', '-E', '-x', 'c', '../../include/config.h'], cwd=cwd, text=True)
    macros = dict(re.findall(r'^#define (ENABLE_\w+|DIAG_LED7|BUZZER_ACTIVE|F_CPU) (\S+)', macros_text, re.M))
    if name in ('Debug', 'Base'):
        assert macros['ENABLE_UART'] == '0'
    objects = []
    for src in sources:
        obj = target / (src.stem + '.o')
        # Source item Include paths are project-relative, NOT compiler-relative.
        relative = '../../src/' + src.name
        subprocess.run([args.compiler] + flags + ['-MMD', '-MP', '-c', relative, '-o', str(obj)], cwd=cwd, check=True)
        assert 'config.h' in obj.with_suffix('.d').read_text()
        objects.append(str(obj))
    subprocess.run([args.compiler, '-mmcu=atmega16', '-Wl,--gc-sections,-Map,' + str(target / 'monitor.map'),
                    *objects, '-o', str(target / 'monitor.elf')], cwd=cwd, check=True)
    prefix = args.compiler.removesuffix('gcc')
    subprocess.run([prefix + 'objcopy', '-O', 'ihex', '-R', '.eeprom', str(target / 'monitor.elf'),
                    str(target / 'monitor.hex')], check=True)
    subprocess.run(['python3', str(root / 'tools/check_size.py'), prefix + 'size', str(target / 'monitor.elf')], check=True)
    reports[name] = {'macros': macros, 'flags': flags, 'cwd': 'studio/' + name,
                     'compiler': version, 'source_count': len(objects)}
(root / args.output / 'verification.json').write_text(json.dumps(reports, indent=2) + '\n')
print('PASS: all', len(reports), 'Studio XML configurations compile/link from configuration working directories;', version)
