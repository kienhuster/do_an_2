# Đấu nối ATmega16, LCD và các module

Sơ đồ chân dưới đây dành cho **ATmega16 DIP-40, nguồn 5 V, thạch anh 8 MHz**.
Đối chiếu ký hiệu mạng trên PCB; không suy ra số chân header J7 từ số chân LCD tiêu chuẩn.
Kit gốc dùng VR1 cho ADC0, không dùng VR1 làm biến trở contrast LCD.

| Chức năng | GPIO / chân DIP-40 | Kết nối |
|---|---|---|
| DHT11 DATA | PA1 / 39 | DATA, kéo lên 5 V bằng 4.7 kΩ; VCC/GND theo module |
| VR1 | PA0 / 40 | Wiper biến trở có sẵn; hai đầu 5 V/GND; firmware đọc ADC0 |
| UP — nút PB1 trên kit | PB0 / 1 | Nút nối GND, active-low |
| DOWN — nút PB2 | PB1 / 2 | Nút nối GND |
| SET — nút PB3 | PB2 / 3 | Nút nối GND |
| ACK/BACK — nút PB4 | PB3 / 4 | Nút nối GND; **không còn ngõ ra động cơ** |
| LCD D4 | PC4 / 26 | LCD chân D4 (chuẩn HD44780 chân 11) |
| LCD D5 | PC5 / 27 | D5 (12) |
| LCD D6 | PC6 / 28 | D6 (13) |
| LCD D7 | PC7 / 29 | D7 (14) |
| LCD RS | PD6 / 20 | RS (4) |
| LCD R/W | PD5 / 19 | R/W (5), firmware luôn xuất LOW |
| LCD E | PD7 / 21 | E (6) |
| LCD nguồn/contrast | 5 V/GND | VDD (2), VSS (1), V0 (3) theo module; dùng biến trở contrast riêng nếu cần |
| UART RX | PD0 / 14 | TX của USB-UART TTL |
| UART TX | PD1 / 15 | RX của USB-UART TTL |
| DS3231 + BH1750 SCL | PC0 / 22 | SCL của cả hai module, pull-up ngoài |
| DS3231 + BH1750 SDA | PC1 / 23 | SDA của cả hai module, pull-up ngoài |
| Buzzer | PD2 / 16 | Điều khiển transistor, active-high; không nối trực tiếp tải lớn vào GPIO |
| Nạp ISP MOSI/MISO/SCK | PB5 / 6, PB6 / 7, PB7 / 8 | Header ISP của kit; thêm RESET/5 V/GND |
| RESET | 9 | Pull-up 10 kΩ; nút PB5 trên kit là reset cứng |
| VCC / AVCC | 10 / 30 | 5 V, tụ 100 nF gần mỗi chân; AVCC phải có nguồn để ADC/PORTA hoạt động |
| GND | 11 / 31 | Chung mass mọi module/programmer/USB-UART |
| AREF | 32 | Tụ 100 nF xuống GND khi dùng AVCC reference; không ép nguồn khác vào AREF |
| XTAL1 / XTAL2 | 13 / 12 | Thạch anh 8 MHz, tụ tải theo thạch anh/kit |

Firmware tự tắt JTAG bằng hai lần ghi liên tiếp để PC2–PC5 dùng được làm GPIO.
PB4/SS và PB5–PB7 được để dành; không có motor driver hoặc PWM động cơ.

## Tránh xung đột với phần cứng kit

1. Trong cấu hình LCD hoặc I²C, **tháo JP2** cấp nguồn chung cho LED 7 đoạn.
   LED 7 đoạn dùng PORTC, trùng LCD/I²C. Firmware chuyển LCD sang 4 bit để giải phóng
   PC0/PC1; các chân LCD D0–D3 có thể để hở trên mạch mới. Nếu kit đã nối chúng sẵn,
   LCD trong chế độ 4 bit bỏ qua chúng và R/W luôn LOW, không xuất dữ liệu lên bus.
2. Với vận hành bình thường dùng UART/LCD/buzzer, tháo JP1 của dãy LED đơn nếu dãy LED
   làm tăng tải GPIO hoặc gây nhiễu. Muốn thử LED đơn thì lắp JP1 sau khi đối chiếu sơ đồ.
   Diagnostic chỉ điều khiển chân trống: base dùng PD2–PD4; full dùng PD3–PD4;
   diagnostic riêng dùng PD2–PD7. PD0/PD1 có thể nháy do UART khi JP1 lắp, không được
   firmware chiếm làm LED diagnostic.
3. Muốn thử toàn bộ LED 7 đoạn: nạp cấu hình `diagnostic`, tháo LCD và các module I²C,
   rồi lắp JP2. Các đoạn lần lượt a=PC5, b=PC4, c=PC2, d=PC1, e=PC0, f=PC6,
   g=PC7, dấu chấm=PC3; anode chung, LOW sáng. Hiển thị vòng 0–9, mỗi bước 300 ms.
4. Tháo mọi mạch công suất/động cơ cũ. Không đưa điện áp động cơ vào kit hay chân ADC.

## Module tùy chọn

DS3231 dùng địa chỉ 7 bit `0x68`. Firmware đọc BCD 24 h hoặc 12 h, kiểm tra ngày hợp lệ
và cờ OSF. Nếu RTC mất nguồn/OSF=1, đồng hồ được coi là chưa hợp lệ; đặt lại bằng lệnh UART.
RTC dùng phạm vi năm 2000–2099. Pin dự phòng theo loại module; một số board có mạch sạc,
phải dùng pin phù hợp với board hoặc xử lý mạch sạc theo hướng dẫn nhà sản xuất.

BH1750 dùng địa chỉ `0x23` khi ADDR LOW; đổi `BH1750_ADDRESS` thành `0x5c` khi ADDR HIGH.
Driver gửi Power On `0x01`, Reset `0x07`, Continuous H-Resolution `0x10`, chờ ít nhất
200 ms lần đầu, đọc MSB/LSB và tính lux = raw/1.2. Chip BH1750 trần dùng nguồn
2.4–3.6 V: **không đưa 5 V trực tiếp vào VCC/SCL/SDA của chip**. Chọn breakout xác nhận
tương thích 5 V hoặc dùng nguồn 3.3 V và bộ chuyển mức I²C hai chiều. DS3231 và BH1750
có thể chia sẻ bus; chọn đúng mức pull-up cho mỗi phía của bộ chuyển mức. Không để các
điện trở kéo lên 5 V có sẵn trên RTC làm quá áp phía BH1750 3.3 V.

Ví dụ buzzer active: PD2 → điện trở base 2.2–4.7 kΩ → transistor NPN, emitter GND,
collector tới cực âm buzzer, cực dương buzzer tới nguồn phù hợp. Thêm điện trở base-GND
để giữ OFF trong reset; với tải cảm cần diode bảo vệ theo loại tải. Buzzer passive dùng
cấu hình `passive`, cùng chân PD2, tone 2 kHz. Firmware không thể xác nhận LCD/buzzer
có được cắm bằng các đường GPIO write-only; việc kiểm tra kết nối của chúng là checklist
quan sát/nghe thực tế. I²C/DHT11 có phản hồi, thiếu thiết bị sẽ báo lỗi.

## Nạp chương trình

1. Kiểm tra nguồn, GND, clock và wiring khi chưa cấp điện; chọn `base` nếu chưa có module tùy chọn.
2. Build, lấy `build/base/monitor.hex` hoặc HEX cấu hình tương ứng trong gói bàn giao.
3. Kết nối USBasp/AVRISP tương thích ISP. Xác nhận signature ATmega16 `0x1E9403`.
4. Đọc/sao lưu fuse và EEPROM trước khi thay firmware đang có cấu hình cần giữ:

```bash
avrdude -p m16 -c usbasp -B 10 -U lfuse:r:lfuse.txt:h -U hfuse:r:hfuse.txt:h
avrdude -p m16 -c usbasp -B 10 -U eeprom:r:backup.eep:i
avrdude -p m16 -c usbasp -B 10 -U flash:w:monitor.hex:i
```

`avrdude` không được chạy với kit thật trong cloud. Trong Studio, Device Programming →
chọn programmer/ATmega16 → đọc signature → Memories → chọn HEX → Program/Verify.

Fuse phải chọn **external crystal 8 MHz**, startup ổn định theo datasheet và mạch nguồn,
SPIEN giữ bật; tránh chọn external clock khi kit chỉ có crystal. Chọn brown-out phù hợp
nguồn 5 V và EESAVE nếu muốn giữ EEPROM qua chip erase. Đối chiếu fuse decoder ATmega16
của programmer trước khi ghi; không có một giá trị fuse đã thử trên kit này để khẳng định.
F_CPU chỉ khai báo cho compiler, không thay đổi fuse hay tần số thực của chip.

Chỉ nạp HEX Flash để chạy. `.eep` được sinh để phân tích/khôi phục có chủ đích; **không nạp
EEP template khi muốn giữ cấu hình**. EEPROM trống hoặc CRC sai dẫn tới cấu hình mặc định
và E:20; dùng menu Save hoặc UART `SAVE` để lưu cấu hình hợp lệ, rồi lỗi này được xóa.
