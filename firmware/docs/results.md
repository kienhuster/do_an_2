# Kết quả build và kiểm thử

Ngày chạy: 2026-10-08. Cloud Debian 13 amd64; MCU ATmega16, F_CPU=8 MHz, GNU99, -Os.

## Dung lượng

| Cấu hình | Flash GCC 14.2 | Flash GCC 5.4 | SRAM tĩnh | EEPROM | Vùng còn cho stack |
|---|---:|---:|---:|---:|---:|
| base | 12674 | 12526 | 490 | 48 | 534 |
| full | 14980 | 14840 | 508 | 48 | 516 |
| rtc | 14498 | 14318 | 508 | 48 | 516 |
| light | 13570 | 13394 | 490 | 48 | 534 |
| passive | 15030 | 14900 | 508 | 48 | 516 |
| diagnostic | 8900 | 8772 | 448 | 48 | 576 |
| minimal | 4342 | 4268 | 143 | 0 | 881 |

Đơn vị byte. ATmega16 có Flash 16,384, SRAM 1,024, EEPROM 512 byte. Cả bảy cấu hình
đã build/link/objcopy thành công với -Wall -Wextra -Werror trên hai toolchain.
GCC 5.4 dùng AVR-Libc 2.0; GCC 14.2 dùng AVR-Libc 2.2.1. Không cần tắt module để full vừa chip.
Full GCC 14.2 còn 1,404 byte Flash; passive còn 1,354 byte. Cần đo lại khi thêm tính năng.

## Kiểm thử đã chạy

- Host C với ASan/UBSan: **308 assertions / 6 nhóm đạt**, không có lỗi sanitizer.
- simavr 1.8, firmware base: **28 assertions đạt**.
- simavr 1.8, firmware full: **38 assertions đạt**, gồm DS3231/BH1750/buzzer, mất/kết nối lại thiết bị.
- simavr 1.8, firmware passive: **38 assertions đạt**, gồm ngõ buzzer chạy Timer0.
- SRAM watermark trên các đường chạy base/full: ít nhất **378/336 byte** chưa bị stack chạm phía trên SRAM tĩnh; đây không phải worst-case proof.
- Hai build bổ sung đạt: DHT11 tắt và toàn bộ module I/O tùy chọn tắt.
- Studio XML đã parse, mọi đường dẫn source/header tồn tại, toolchain C đúng; GCC 5.4 đã build bảy profile tương ứng.
- Setup script chạy với refresh dependency thành công, APT kiểm tra signature/hash, base/full build và mô phỏng đạt.
- Git read-only qua origin thành công; không cần thêm credential hay mở domain ngoài preset hiện có.
- Các tệp nguồn gốc và bản sao backup đối chiếu byte-for-byte; tracked files gốc không đổi.

**Chưa chạy:** Microchip/Atmel Studio GUI trên Windows, programmer/ISP, kit thật hoặc thiết bị serial thật.
Mọi chức năng đều đã viết C; kết quả mô phỏng không chứng minh các mạch thật hoạt động.
Xem [validation.md](validation.md) để biết chi tiết fixture simavr và checklist thử kit.

## HEX bàn giao — AVR-GCC 14.2

| HEX | SHA256 |
|---|---|
| base.hex | `54bcc6e17d6d4bb5f1e49fbb1aa891362a43751afffb41c8d03ba44ed4792bc6` |
| full.hex | `16b6c77ea4426b744b71f843e354cb1c1c862a9979edb61fde9fbd3c68304bfd` |
| rtc.hex | `186e94df8dbdc5b6d64df930e77d07ad467be2891a935d5a6a5b5598558f003e` |
| light.hex | `78b2918c6a6fdafd1a6dae076be36d2c96773cb11abb7e8a0118c0e872f636c8` |
| passive.hex | `79c1c7273952f2172dc072ae541a64aa5c3ec389d878036e4acdde3112151ecf` |
| diagnostic.hex | `10db9b32344a6a710b49147f4bc73989bacab8f493492f392ca3b01b7676badc` |
| minimal.hex | `e01b1a2778ae12570d5aa91a2160aee24f5fc1c3ceea81e1b5134fd35beb8a0b` |

Gói ZIP có HEX hai toolchain trong các thư mục riêng. `.eep` không nên được nạp nếu muốn giữ cấu hình.

## Môi trường cloud

Đã lưu bản nháp `install_script` và `start_skill` cho checkout hiện có. Không thêm secrets,
biến môi trường ứng dụng hoặc thay đổi network allowlist. Công cụ được giữ dưới `/workspace/.avr`;
activation tạo PATH/library/include cho build và simulator. Không có dịch vụ nền cần khởi động.

Bản nháp và việc chuẩn bị máy hiện tại **chưa phải Publish**, và chưa kiểm chứng khôi phục trong task mới.
Người dùng xem/lưu thay đổi trong Environment settings rồi Publish để tạo snapshot môi trường.
