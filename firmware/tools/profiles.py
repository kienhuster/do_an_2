"""One profile definition shared by Make and the Studio project generator."""
import re
import sys

PROFILES = {
    'base': {},
    'uart': {'ENABLE_UART': 1},
    'full': {'ENABLE_UART': 1, 'ENABLE_BUZZER': 1, 'ENABLE_RTC': 1, 'ENABLE_BH1750': 1},
    'rtc': {'ENABLE_UART': 1, 'ENABLE_RTC': 1},
    'light': {'ENABLE_UART': 1, 'ENABLE_BH1750': 1},
    'passive': {'ENABLE_UART': 1, 'ENABLE_BUZZER': 1, 'ENABLE_RTC': 1,
                'ENABLE_BH1750': 1, 'BUZZER_ACTIVE': 0},
    'diagnostic': {'ENABLE_UART': 1, 'ENABLE_LCD': 0, 'ENABLE_BUZZER': 0,
                   'ENABLE_RTC': 0, 'ENABLE_BH1750': 0, 'DIAG_LED7': 1},
    'minimal': {'ENABLE_LCD': 0, 'ENABLE_UART': 0, 'ENABLE_EEPROM': 0,
                'ENABLE_ADC': 0, 'ENABLE_DIAGNOSTIC': 0},
}
STUDIO = {'Debug': 'base', 'Release': 'full', 'Base': 'base', 'UART': 'uart',
          'Full': 'full', 'RTC': 'rtc', 'Light': 'light', 'Passive': 'passive',
          'Diagnostic': 'diagnostic', 'Minimal': 'minimal'}

def definitions(profile, overrides=()):
    values = dict(PROFILES[profile])
    for flag in overrides:
        match = re.fullmatch(r'-D([A-Za-z_]\w*)(?:=(\w+))?', flag)
        if not match:
            raise ValueError('EXTRA_DEFS accepts only -DNAME=value: ' + flag)
        values[match[1]] = match[2] or '1'
    return ['-D' + k + '=' + str(v) for k, v in values.items()]

if __name__ == '__main__':
    try:
        print(' '.join(definitions(sys.argv[1], sys.argv[2:])))
    except (KeyError, ValueError) as error:
        sys.exit(str(error))
