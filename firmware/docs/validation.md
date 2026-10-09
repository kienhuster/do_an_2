# Kiến trúc và kiểm thử

## Tổ chức mã nguồn

| Tệp | Trách nhiệm |
|---|---|
| `main.c` / `app.h` | Điều phối, uptime, chu kỳ cảm biến, lỗi, buzzer và trạng thái chung |
| `config.h` | Bật/tắt module và chặn xung đột chân |
| `core.c` / `core.h` | Logic độc lập phần cứng: ngưỡng, hysteresis, ACK, thống kê, xu hướng, CRC, codec RTC/DHT, parse số |
| `board.c` | GPIO, Timer1, nút, ADC, buzzer, LED diagnostic |
| `reset.c` | Lưu MCUCSR và tắt watchdog sớm trong `.init3` |
| `lcd.c` | HD44780 4 bit, ghi đủ 16 ký tự mỗi dòng để xóa ký tự cũ |
| `dht.c` | Start open-drain, đọc xung bằng Timer2, timeout và kiểm tra checksum |
| `uart.c` / `command.c` | RX/TX ring, ISR UART, lệnh hai chiều và CSV |
| `storage.c` | Hai slot EEPROM, commit cuối, CRC và đọc xác minh |
| `i2c.c` / `optional.c` | TWI 100 kHz, repeated START, ACK/NACK, timeout/recovery; DS3231/BH1750 |
| `ui.c` | Trang LCD, menu bốn nút, sửa cấu hình tạm trước khi Save |

Timer1 chạy tự do 1 MHz: overflow mỗi 65.536 ms cập nhật phần nguyên và dư của clock.
Compare A cách nhau 10,000 tick quét nút; chống dội vẫn chạy khi main đang truyền UART
hoặc ghi EEPROM. Khi đọc DHT, ngắt bị khóa khoảng 4 ms; clock vẫn tính phần counter/
overflow đang chờ nên không bỏ mất nhiều millisecond mỗi lần đọc. Timer2 chạy 1 MHz,
đo xung DHT modulo 256 µs; mỗi pha có timeout 100 µs. Timer0 chỉ dùng trong cấu hình
buzzer passive, compare 4 kHz và đảo PD2 tạo tone 2 kHz; không có PWM động cơ.

Start DHT kéo LOW được chia thành hai bước không khóa main trong 20 ms. Trong khung
DHT, UART RX được poll để giảm mất dữ liệu khi ngắt bị khóa. Vòng lặp main có watchdog
1 s; ADC và TWI đều có timeout. TX dùng ring 128 byte, RX 64 byte và command buffer
48 byte, không xử lý command trong ISR. Không malloc/new, không floating-point printf.
Các chuỗi UI/UART dài nằm trong Flash bằng PROGMEM/PSTR.

Các trường hợp như HELP hoặc SAVE vẫn có thể làm main bận vài chục tới vài trăm ms;
quét phím và nhận UART chạy bằng ngắt, sự kiện được giữ đến khi main xử lý. Nhiều lần
nhấn cùng một nút trong khoảng main bận có thể được gộp thành một sự kiện. Đây không
phải hệ điều hành thời gian thực; firmware dành cho hệ giám sát mẫu 2 s.

## Tính toàn vẹn EEPROM

Hai slot, mỗi slot 24 byte: commit marker, schema, sequence 32 bit, settings, reserved
và CRC16-CCITT (`poly 0x1021`, init `0xFFFF`). Khi Save: vô hiệu slot đích → ghi payload/
CRC → ghi marker cuối → đọc lại và so toàn bộ bytes. Slot trước đó được giữ để mất nguồn
giữa Save vẫn có thể nạp cấu hình cũ. Sequence so theo modulo 32 bit để xử lý wrap.
Không ghi định kỳ hoặc theo mỗi mẫu; chỉ Save trong menu/lệnh, hạn chế mòn EEPROM.
Số liệu đo, xu hướng, uptime và trạng thái ACK không được lưu qua reset.

## Bằng chứng kiểm thử và giới hạn

`tests/test_core.c` kiểm tra logic bằng compiler host với AddressSanitizer/UBSan:
ngưỡng/giới hạn/narrowing, parse đầu vào, bốn loại alarm và hysteresis, ACK/rearm,
MIN/MAX/AVG/xu hướng/counter limit, CRC và mọi single-bit corruption của record,
mô hình mất nguồn ở từng bước commit, sequence wrap, BCD/ngày nhuận/12 h/century,
checksum/range DHT và wrap timestamp. Kết quả/count cụ thể trong `results.md`.

`tests/sim_smoke.c` chạy chính ELF ATmega16, không thay driver: mô hình ngoài phát
xung DHT 27/70 µs, giải mã cạnh LCD, truyền UART, cấp ADC 2500 mV và trả lời TWI.
Kiểm tra command/EEPROM, ngưỡng high, ACK, hysteresis clear/rearm, checksum sai,
cảm biến mất kết nối và khôi phục, line overflow/framing error, debounce, cấu hình
còn sau watchdog reset; cấu hình full thêm RTC read/write/date validation/OSF/EOSC,
lux, buzzer và tháo/lắp lại I²C. Cuối test, chèn vòng lặp vô hạn trong vùng Flash trống
**của simulator** để watchdog reset thật trong mô phỏng; HEX xuất xưởng không bị chèn mã.

simavr Debian 1.6/v1.7 có lỗi trạng thái/clear TWINT gây thất bại driver TWI đúng giao
thức; môi trường sử dụng upstream **simavr v1.8**, commit cố định trong setup script.
v1.8 còn có lỗi metadata IRQ của watchdog reset-only trên ATmega16: khi reset lần hai,
bộ mô phỏng dereference pool NULL. Fixture chỉ bổ sung pool pointer cho pseudo-IRQ
watchdog không có vector, bằng cấu trúc công khai của simavr. Không thay thanh ghi
TWI, không sửa firmware để chấp nhận status sai, không tắt assertion/watchdog.
Phần trace có thể bật bằng `TWI_TRACE=1 make PROFILE=full simulate`.

Mô hình RTC giữ các thanh ghi thời gian được cấp, không chứng minh sai số/hoạt động pin
dự phòng thực; I²C model không chứng minh dạng sóng, điện áp hoặc clock stretching thực.
Các GPIO write-only của LCD/buzzer không có cơ chế tự nhận biết dây chưa cắm. ADC timeout
phát hiện conversion lỗi, không phân biệt VR1 hở dây với một điện áp ADC hợp lệ.

Báo cáo `.size.json` là SRAM tĩnh; vùng còn lại phải dành cho stack/ISR. Compiler xuất
`.su` cho các hàm thông thường (hàm startup naked không hỗ trợ tính stack). Test mô phỏng
đo watermark trên các đường chạy đã thử, **không phải chứng minh worst-case stack**.
Giới hạn build: Flash <=16,384 byte, EEPROM <=512 byte, SRAM tĩnh <=768 byte để giữ
ít nhất 256 byte stack. Hiện full/passive đều nằm trong giới hạn; xem số thực ở results.

## Checklist phần cứng trước bảo vệ

Tất cả mục dưới đây **chưa được kiểm thử trên kit thật**. Ghi người thử, ngày, wiring,
profile, HEX SHA256 và kết quả; không đánh dấu đạt chỉ vì build/mô phỏng đã đạt.

| Nhóm | Thử thực tế | Tiêu chí |
|---|---|---|
| Nguồn/clock/fuse | Đo 5 V/AVCC, oscillator 8 MHz, đọc signature/fuse | Nguồn ổn định, đúng chip, ISP Verify đạt |
| DHT11 | Đo start/response/0/1 bằng logic analyzer; so với nhiệt ẩm kế | Đúng timing/checksum, số đo trong sai số sensor; không nhầm DHT22 |
| DHT lỗi | Rút DATA/nguồn, chập mức qua cấu hình thử an toàn, cắm lại | Dữ liệu trống, lỗi sau ba lần, tự hồi phục, không treo |
| LCD | Power cycle, contrast, mọi trang và menu | Không rác/ký tự dư; PC4–PC7/PD5–PD7 đúng |
| Bốn nút | Nhấn nhanh, dội, giữ, nhấn lúc SAVE/UART | Một sự kiện mỗi lần; cancel không lưu, Save lưu |
| Alarm | Đi qua từng ngưỡng ở cả hai chiều; chỉnh ngưỡng qua menu/UART | Đúng bốn bit, hysteresis, ACK và tái cảnh báo |
| EEPROM | Save/load/reset; mất nguồn có kiểm soát giữa Save | Bản mới hoặc bản cũ hợp lệ, không chấp nhận payload hỏng |
| Thống kê/trend | Tập mẫu nhiệt/ẩm tăng, giảm, ổn định; RESETSTATS | Đối chiếu MIN/MAX/AVG, đủ tám mẫu mới có trend |
| UART/CSV | USB-UART TTL, lệnh lỗi/quá dài, ghi file nhiều phút | 9600 8N1, 11 cột, không nhận command bị cắt, trường lỗi trống |
| Uptime | So với đồng hồ nhiều giờ/ngày; DHT/UART hoạt động đồng thời | Sai số theo clock thực, không mất tick do critical section |
| ADC VR1 | Đo min/mid/max bằng đồng hồ, kiểm AVCC | Mã gần 0/512/1023; điện áp hiển thị theo reference đã biết |
| Buzzer active/passive | Test transistor, SOUND/ACK, alarm mới | Active chirp hoặc passive 2 kHz, tắt sau ACK, tái kêu đúng |
| RTC | Đặt giờ, ngày nhuận, rút module, tháo nguồn chính có pin | OSF/thiếu module báo lỗi, đọc lại đúng, giữ giờ bằng pin |
| BH1750 | Kiểm mức logic; tối/sáng, ADDR, rút/cắm | Lux thay đổi hợp lý, init/chờ conversion, tự retry |
| Diagnostic | JP1/JP2 và profile tương ứng, LED 0–9 | Chân UART/LCD/I²C/buzzer không bị chiếm; reset PB5 đúng |
| Watchdog/lỗi | Treo có kiểm soát bằng firmware thử riêng; reset nguồn/brown-out | Reset ~1 s khi treo, WDRF ghi nhận, EEPROM vẫn hợp lệ |
| Tích hợp | Chạy full nhiều giờ, nhận CSV và thao tác đồng thời | Không reset bất thường, không tràn stack/buffer, không mất sensor |

## Nguồn giao thức

- [ATmega16 datasheet, Microchip](https://ww1.microchip.com/downloads/en/DeviceDoc/doc2466.pdf): GPIO, timer, USART, ADC, EEPROM, TWI, watchdog và fuse.
- [DS3231 datasheet, Analog Devices](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf): registers 00–06, control 0E, status 0F, OSF/EOSC.
- [BH1750FVI datasheet, ROHM](https://fscdn.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1750fvi-e.pdf): power/reset/continuous H-resolution và 1.2 conversion.
- Tài liệu kit và mã DHT11 gốc trong repository: sơ đồ chân, 8 MHz, ADC0, PB0–PB3, LCD 1602.
- [simavr upstream](https://github.com/buserror/simavr): phiên bản và lỗi mô phỏng được phân biệt với lỗi firmware.
