# Hệ thống giám sát môi trường ATmega16 — Embedded C

Project mới triển khai 17 nhóm chức năng được yêu cầu bằng AVR-GCC/AVR-Libc, chuẩn GNU99.
`src/main.c` chứa `main()`. Không có Arduino, C++, heap, động cơ hay PWM động cơ.
Hai tệp gốc ở thư mục repository được giữ nguyên; bản sao nằm trong `backup/`.
Repository ban đầu không có `GccApplication2.cppproj` hay project Studio để chuyển đổi.
Tài liệu báo cáo cũ mô tả DHT22/động cơ; project này dùng **DHT11**, ánh xạ theo kit trong
“Merged - Tai lieu huong dan.pdf” và theo yêu cầu mới. Không dùng báo cáo cũ làm hướng dẫn đấu nối cho firmware mới.

## Bắt đầu

Trên môi trường cloud đã thiết lập:

```bash
cd /workspace/do_an_2/firmware
source /workspace/.avr/activate.sh
make -j2 PROFILE=base
make -j2 matrix
make test
make PROFILE=base simulate
make PROFILE=full simulate
```

Để cài lại các công cụ trên cloud Debian amd64, chạy `bash tools/setup_cloud.sh`.
Script dùng các gói Debian được APT xác minh chữ ký và checksum, cài riêng dưới `/workspace/.avr`;
simavr 1.8 được lấy từ commit upstream cố định. Không cần sudo, token mới hoặc dịch vụ nền.
Mỗi task đã có môi trường riêng: sử dụng checkout hiện có, không tạo worktree.

Trên máy Linux khác có AVR-GCC, AVR-Libc, AVR Binutils, Make và Python 3, chạy `make` trong thư mục này.
Các test host cần compiler C có AddressSanitizer/UBSan. Mô phỏng cần simavr 1.8 và `libelf`;
đặt `SIM_CFLAGS`/`SIM_LDFLAGS` theo nơi cài nếu không có header/library mặc định.

Trên Windows mở `studio/MonitorATmega16.cproj` trong Microchip Studio/Atmel Studio 7,
chọn ATmega16 và toolchain **AVR/GNU C Compiler**. `Debug` tương đương `Base`, `Release`
tương đương `Full`; các cấu hình Base/Full/RTC/Light/Passive/Diagnostic/Minimal cũng được khai báo.
Nếu IDE chưa hiện các cấu hình tùy chỉnh, thêm tên tương ứng trong Configuration Manager.
Các tệp `.c` được biên dịch bằng AVR-GCC; header được khai báo trong project.
Compiler phải có `-std=gnu99`, `-Os`, `F_CPU=8000000UL`, include `../include`, và linker
`--gc-sections`. Có thể tái tạo XML bằng `python3 tools/generate_studio.py`.

**Đã kiểm tra compiler AVR-GCC 14.2 và 5.4 trên Linux. Chưa chạy IDE Studio trên Windows**;
file project được kiểm tra XML, danh sách nguồn và cấu hình. Kết quả này không thay thế việc
mở/build project bằng IDE trên máy Windows của bạn.

## Các cấu hình

| `PROFILE` | Thành phần |
|---|---|
| `base` | DHT11, LCD, nút, EEPROM, UART/CSV, ADC, thống kê, cảnh báo, watchdog, diagnostic chân trống |
| `full` | Base + buzzer active + DS3231 + BH1750 |
| `rtc` | Base + DS3231 |
| `light` | Base + BH1750 |
| `passive` | Full, dùng buzzer passive với sóng 2 kHz từ Timer0 CTC |
| `diagnostic` | LCD/I²C/buzzer tắt; LED 7 đoạn + các LED đơn không chiếm UART; tự vào diagnostic |
| `minimal` | DHT11, xử lý dữ liệu/cảnh báo, nút và watchdog; không LCD/UART/ADC/EEPROM |

Tùy chọn nằm ở `include/config.h`, có thể bật bằng `#define` hoặc `-D`.
Các giá trị mặc định cho buzzer/RTC/BH1750 là 0; **driver đầy đủ đã có trong `src/optional.c`
và `src/i2c.c`**, và được biên dịch trong các cấu hình tương ứng. Các nhánh module tắt trả về
“không khả dụng”; chúng không sinh dữ liệu giả. Dữ liệu mô phỏng chỉ nằm trong `tests/`.

Ví dụ build tùy chỉnh:

```bash
make PROFILE=base OUT=build/custom EXTRA_DEFS='-DENABLE_RTC=1 -DENABLE_LCD=0'
```

Makefile ghi lại flags và tự build lại khi compiler/flags thay đổi. Thay đổi config.h cũng
làm build lại. `DIAG_LED7=1` cùng LCD hoặc I²C bị chặn bằng lỗi biên dịch để tránh tranh chân.
Clock hiện hỗ trợ **8 MHz**; đổi `F_CPU` thành tần số khác sẽ báo lỗi, vì phải thiết kế lại timing.

## Tài liệu và kết quả

- [Đấu nối và nạp chương trình](docs/wiring.md), [sơ đồ SVG](docs/wiring.svg).
- [Hướng dẫn sử dụng và giao thức](docs/usage.md).
- [Kiến trúc, giới hạn và kiểm thử phần cứng](docs/validation.md).
- [Báo cáo dung lượng và kết quả chạy](docs/results.md).
- Mỗi cấu hình có `build/<profile>/monitor.hex`, `.elf`, `.map`, `.eep`, `.size.json`.
- `tools/capture_csv.py` thu CSV trên máy tính qua USB-UART; chỉ công cụ PC dùng Python.

HEX đã sinh được trên cloud. **Chưa có kết quả thử với kit thật**, kể cả DHT11, LCD,
I²C, ADC, buzzer, fuse và programmer. Xem checklist trước khi trình diễn đồ án.
