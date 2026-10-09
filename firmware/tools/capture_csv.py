"""PC-side CSV capture; install pyserial (firmware remains pure C)."""
import argparse
import csv
import time
import sys

parser = argparse.ArgumentParser()
parser.add_argument('port', help='COM3 on Windows or /dev/ttyUSB0 on Linux')
parser.add_argument('--output', default='monitor.csv')
parser.add_argument('--seconds', type=float, default=60)
args = parser.parse_args()
try:
    import serial
except ImportError:
    sys.exit('Install pyserial: python -m pip install pyserial==3.5')
header = 'uptime_s,temp_c,humidity_pct,t_trend,h_trend,alarm,ack,errors,adc_raw,lux,rtc'.split(',')
with serial.Serial(args.port, 9600, timeout=1) as port, open(args.output, 'w', newline='') as out:
    writer = csv.writer(out)
    writer.writerow(header)
    port.write(b'CSV ON\n')
    end = time.monotonic() + args.seconds
    count = 0
    while time.monotonic() < end:
        line = port.readline().decode('ascii', errors='replace').strip()
        if not line or line.startswith('#') or not line[0].isdigit():
            continue
        row = next(csv.reader([line]))
        if len(row) == len(header):
            writer.writerow(row)
            out.flush()
            count += 1
    print(f'Saved {count} records to {args.output}')
