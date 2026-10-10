# Các cấu hình build

Ngày xác minh: 2026-10-10. GNU99, ATmega16, F_CPU=8000000UL; không đổi fuse/clock.

## Tính năng và linh kiện

| Make PROFILE | Studio | UART | Thành phần |
|---|---|---:|---|
| base | Base / Debug | 0 | DHT11, LCD, 4 nút, EEPROM, ADC, cảnh báo, thống kê/xu hướng, uptime, watchdog; diagnostic chân trống |
| uart | UART | 1 | Base + UART hai chiều/CSV |
| full | Full / Release | 1 | UART + buzzer active + DS3231 + BH1750 |
| rtc | RTC | 1 | UART + DS3231 |
| light | Light | 1 | UART + BH1750 |
| passive | Passive | 1 | Full với buzzer passive Timer0 2 kHz |
| diagnostic | Diagnostic | 1 | DHT11, nút, EEPROM, ADC, UART/CSV, LED7/LED đơn; LCD/RTC/BH1750/buzzer OFF |
| minimal | Minimal | 0 | DHT11, nút, xử lý dữ liệu/cảnh báo, uptime/watchdog; LCD/UART/EEPROM/ADC/diagnostic OFF |

Bản Base phù hợp linh kiện hiện có. Base/Debug dùng mặc định config.h UART=0; nếu bạn chủ động sửa mặc định thành 1, chúng sẽ theo thay đổi đó. Full/RTC/Light/Passive/Diagnostic có -DENABLE_UART=1 rõ ràng. Tắt UART trong một bản mở rộng bằng EXTRA_DEFS=-DENABLE_UART=0 trên Make, hoặc thay giá trị duy nhất trong Symbols của cấu hình Studio. Không thêm một macro cùng tên lần thứ hai. UART vẫn được triển khai đầy đủ.

`tools/profiles.py` là nguồn định nghĩa chung; generator và Make đều đọc nó. Các `-D` của profile override mặc định #ifndef trong config.h. EXTRA_DEFS được hợp nhất theo tên và override profile chỉ một lần. Muốn tùy chọn thành mặc định cho mọi profile, kiểm tra cả những profile có override rõ ràng; không suy rằng chỉ sửa config.h sẽ thay được một -D.

## Các -D ngoài F_CPU và mặc định config.h

| Profile | Flags |
|---|---|
| base | `(không có)` |
| uart | `-DENABLE_UART=1` |
| full | `-DENABLE_UART=1 -DENABLE_BUZZER=1 -DENABLE_RTC=1 -DENABLE_BH1750=1` |
| rtc | `-DENABLE_UART=1 -DENABLE_RTC=1` |
| light | `-DENABLE_UART=1 -DENABLE_BH1750=1` |
| passive | `-DENABLE_UART=1 -DENABLE_BUZZER=1 -DENABLE_RTC=1 -DENABLE_BH1750=1 -DBUZZER_ACTIVE=0` |
| diagnostic | `-DENABLE_UART=1 -DENABLE_LCD=0 -DENABLE_BUZZER=0 -DENABLE_RTC=0 -DENABLE_BH1750=0 -DDIAG_LED7=1` |
| minimal | `-DENABLE_LCD=0 -DENABLE_UART=0 -DENABLE_EEPROM=0 -DENABLE_ADC=0 -DENABLE_DIAGNOSTIC=0` |

BH1750_ADDRESS mặc định 0x23, có thể override 0x5c; chỉ hai giá trị này hợp lệ. DIAG_LED7 với LCD/RTC/BH1750 bị chặn lúc preprocessing. Không có PWM/driver động cơ; Timer0 chỉ tạo tone buzzer passive, Timer1 dành clock/nút, Timer2 dành DHT.

## Build trên cloud/Linux

```sh
source /workspace/.avr/activate.sh
make -C firmware PROFILE=base
make -C firmware matrix
make -C firmware PROFILE=full OUT=build/full-uart-off EXTRA_DEFS=-DENABLE_UART=0
make -C firmware PROFILE=base OUT=build/legacy-base AVR_PREFIX=/workspace/.avr/legacy/root/usr/bin/avr-
```

`legacy-base`, `legacy-full`, `legacy-rtc`, `legacy-light`, `legacy-passive`, `legacy-diagnostic`, `legacy-minimal`, `legacy-uart` là thư mục output/toolchain GCC 5.4, **không phải tên PROFILE riêng**. Dùng PROFILE gốc, AVR_PREFIX tương ứng và OUT=build/legacy-<profile>. Giữ các output Studio, GCC 14.2 và GCC 5.4 riêng để không tái sử dụng object của cấu hình khác. `.flags` tự nhận biết thay đổi compiler/flags; header dependency kích hoạt rebuild.

## Dung lượng byte sau khi build lại

| Profile | Flash 14.2 | Flash 5.4 | SRAM tĩnh | EEPROM | Còn cho stack |
|---|---:|---:|---:|---:|---:|
| base | 8690 | 8638 | 194 | 48 | 830 |
| uart | 12696 | 12550 | 490 | 48 | 534 |
| full | 15002 | 14864 | 508 | 48 | 516 |
| rtc | 14520 | 14342 | 508 | 48 | 516 |
| light | 13592 | 13418 | 490 | 48 | 534 |
| passive | 15052 | 14924 | 508 | 48 | 516 |
| diagnostic | 8920 | 8794 | 448 | 48 | 576 |
| minimal | 3242 | 3222 | 143 | 0 | 881 |

Giới hạn chip: Flash 16384, SRAM 1024, EEPROM 512. Build từ chối SRAM tĩnh >768 để chừa ít nhất 256 byte stack. Full còn 1382 byte Flash với GCC 14.2, Passive còn 1332. Cột SRAM không bao gồm stack; watermark mô phỏng không chứng minh mọi đường chạy.

## Nguồn gốc HEX

Bản dễ chọn cho Windows nằm trong `releases/windows-gcc54/ATmega16_DHT11_<tên>.hex`, đặc biệt `ATmega16_DHT11_BASE_UART_OFF.hex`. Tất cả được copy từ bản **vừa compile/link/objcopy**, không đổi tên HEX cũ. `releases/BUILD_MANIFEST.json` ghi source SHA256, compiler, flags thực, macro sau preprocessing, dung lượng và hash HEX/ELF cho 16 bản. `releases/SHA256SUMS.txt` kiểm toàn bộ artifact. Cùng tên monitor.hex trong mỗi thư mục đáp ứng Make/debug; tên bàn giao rõ ràng không thay output gốc.

`releases/` là snapshot; khi tự sửa source/build GUI phải dùng output mới và cập nhật snapshot có chủ đích. Không xem HEX snapshot là đã theo source sau bất kỳ sửa đổi nào. Xem BUILD_FIX_REPORT.md để biết bản nào được mô phỏng, bản nào chỉ build; chưa có bản nào được thử trên kit thật.
