# Bàn giao project ATmega16 Monitor C

Các kết quả có sẵn được giữ nguyên; lần bàn giao này không viết lại firmware.

## Tải về Windows

1. Mở nhánh `atmega16-dht11-monitor-complete` trên GitHub.
2. Chọn **Code → Download ZIP**, rồi giải nén. Hoặc mở file `ATmega16_Monitor_C.zip` tại thư mục gốc và chọn **Download raw file** để tải gói đã đóng sẵn.
3. Mở `firmware/studio/MonitorATmega16.cproj` bằng Microchip Studio. Đọc hướng dẫn đấu nối trước khi cấp nguồn và nạp chương trình.

Có thể dùng Git for Windows:

```sh
git clone --branch atmega16-dht11-monitor-complete --single-branch https://github.com/kienhuster/do_an_2.git
```

## Các kết quả bàn giao

- [Project đóng gói](ATmega16_Monitor_C.zip). SHA256: `2c641760b1534daea4e5d2fbfcd4e5eaa7d8ddb14a5adc5e93fdf4df2e24b38e`.
- [Hướng dẫn project](firmware/README.md), [đấu nối](firmware/docs/wiring.md), [sử dụng](firmware/docs/usage.md), [sơ đồ](firmware/docs/wiring.svg).
- [Báo cáo kiểm thử và dung lượng](firmware/docs/results.md), [kế hoạch kiểm thử phần cứng](firmware/docs/validation.md).
- `firmware/src/*.c` và `firmware/include/*.h`: mã nguồn Embedded C.
- `firmware/releases/`: bản sao nguyên vẹn HEX, ELF, MAP, EEPROM và báo cáo dung lượng đã biên dịch cho 7 cấu hình, với hai phiên bản AVR-GCC. Các bản này không bị xóa khi chạy `make clean`.
- `firmware/releases/SHA256SUMS.txt`: checksum các bản build bàn giao.
- `firmware/backup/`: bản sao nguyên vẹn mã nguồn gốc. Các tệp gốc ở thư mục gốc vẫn được giữ nguyên.

`base`, `full`, `rtc`, `light`, `passive`, `diagnostic`, `minimal` dùng AVR-GCC 14.2. Các thư mục có tiền tố `legacy-` dùng AVR-GCC 5.4. Chọn đúng cấu hình theo linh kiện thực tế; xem README và bảng đấu nối trước khi dùng `full`.

Firmware đã được biên dịch và kiểm thử logic/mô phỏng theo báo cáo. Chưa kiểm thử trên phần cứng thật và chưa chạy giao diện Microchip Studio trên Windows. Không xem kết quả biên dịch hoặc mô phỏng là xác nhận phần cứng hoạt động.
