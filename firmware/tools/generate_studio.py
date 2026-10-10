"""Generate a native AVR-GCC C project; no Arduino/C++ dependencies."""
from pathlib import Path
import xml.etree.ElementTree as ET
from profiles import STUDIO, definitions

root_dir = Path(__file__).resolve().parent.parent
ns = 'http://schemas.microsoft.com/developer/msbuild/2003'
ET.register_namespace('', ns)

def add(parent, name, text=None, **attrs):
    element = ET.SubElement(parent, f'{{{ns}}}{name}', attrs)
    if text is not None:
        element.text = str(text)
    return element

project = ET.Element(f'{{{ns}}}Project', {'DefaultTargets': 'Build', 'ToolsVersion': '14.0'})
props = add(project, 'PropertyGroup')
for name, value in {
    'SchemaVersion': '2.0', 'ProjectVersion': '7.0',
    'ProjectGuid': '{920D1ED4-F53E-4E48-BC00-52A045E416AC}',
    'OutputType': 'Executable', 'Name': 'MonitorATmega16',
    'ToolchainName': 'com.Atmel.AVRGCC8.C', 'ToolchainFlavour': 'Native',
    'Device': 'ATmega16', 'avrdevice': 'ATmega16', 'Architecture': 'AVR8',
    'avrdeviceexpectedsignature': '0x1E9403', 'avrtool': 'com.atmel.avrdbg.tool.simulator',
    'OutputFileName': '$(MSBuildProjectName)', 'OutputFileExtension': '.elf',
    'Language': 'C',
}.items():
    add(props, name, value)
add(props, 'Configuration', 'Base', Condition="'$(Configuration)' == ''")
add(props, 'Platform', 'AVR', Condition="'$(Platform)' == ''")
for name, profile in STUDIO.items():
    group = add(project, 'PropertyGroup', Condition=f"'$(Configuration)' == '{name}'")
    # Compiler runs in studio/<Configuration>, unlike project file item paths.
    add(group, 'OutputPath', f'{name}\\')
    tool = add(add(group, 'ToolchainSettings'), 'AvrGcc')
    for key, value in {
        'avrgcc.common.Device': '-mmcu=atmega16',
        'avrgcc.common.outputfiles.hex': 'True',
        'avrgcc.common.outputfiles.lss': 'True',
        'avrgcc.common.outputfiles.eep': 'True',
        'avrgcc.common.outputfiles.map': 'True',
        'avrgcc.compiler.optimization.level': 'Optimize for size (-Os)',
        'avrgcc.compiler.optimization.PrepareFunctionsForGarbageCollection': 'True',
        'avrgcc.compiler.optimization.PrepareDataForGarbageCollection': 'True',
        'avrgcc.compiler.warnings.AllWarnings': 'True',
        'avrgcc.compiler.warnings.ExtraWarnings': 'True',
        'avrgcc.compiler.miscellaneous.OtherFlags': '-std=gnu99 -Werror',
        'avrgcc.linker.optimization.GarbageCollectUnusedSections': 'True',
    }.items():
        add(tool, key, value)
    defs = add(add(tool, 'avrgcc.compiler.symbols.DefSymbols'), 'ListValues')
    for value in ['F_CPU=8000000UL'] + [d[2:] for d in definitions(profile)]:
        add(defs, 'Value', value)
    includes = add(add(tool, 'avrgcc.compiler.directories.IncludePaths'), 'ListValues')
    add(includes, 'Value', '..\\..\\include')
items = add(project, 'ItemGroup')
for folder, pattern, subtype in [('src', '*.c', 'compile'), ('include', '*.h', 'compile')]:
    for file in sorted((root_dir / folder).glob(pattern)):
        add(add(items, 'Compile', Include=f'..\\{folder}\\{file.name}'), 'SubType', subtype)
add(project, 'Import', Project=r'$(AVRSTUDIO_EXE_PATH)\Vs\Compiler.targets')
ET.indent(project)
ET.ElementTree(project).write(root_dir / 'studio' / 'MonitorATmega16.cproj', encoding='utf-8', xml_declaration=True)
