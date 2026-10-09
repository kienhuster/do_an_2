import subprocess, sys, json
sections = {}
for line in subprocess.check_output([sys.argv[1], '-A', sys.argv[2]], text=True).splitlines():
    fields = line.split()
    if len(fields) > 1 and fields[0].startswith('.'):
        sections[fields[0]] = int(fields[1])
flash = sections.get('.text', 0) + sections.get('.data', 0)
sram = sections.get('.data', 0) + sections.get('.bss', 0) + sections.get('.noinit', 0)
eeprom = sections.get('.eeprom', 0)
print(f'Flash {flash}/16384 bytes; static SRAM {sram}/1024; EEPROM {eeprom}/512; stack headroom {1024-sram}')
with open(sys.argv[2].replace('.elf', '.size.json'), 'w') as f:
    json.dump(dict(flash=flash, static_sram=sram, eeprom=eeprom, stack_headroom=1024-sram), f, indent=2)
if flash > 16384 or sram > 768 or eeprom > 512:
    sys.exit('Resource limit exceeded (reserve at least 256 bytes for stack)')
