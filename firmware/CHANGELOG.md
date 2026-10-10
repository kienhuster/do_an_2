# Changelog

## 2026-10-10 — fix/atmega16-studio-build-and-firmware

- Sửa include Studio từ `../include` sang `../../include`, đồng bộ OutputPath theo cấu hình. Giữ đường dẫn item XML theo thư mục project. Sửa generator, kiểm tra 10 cấu hình bằng hai compiler từ cwd Studio tương ứng.
- Base/Debug mặc định UART OFF; thêm profile UART riêng. Các bản mở rộng giữ UART ON. Loại tác vụ, bộ đệm, parser/CSV/ISR khỏi binary OFF; tắt UCSRB lúc init kể cả khi bootloader để UART bật.
- Chia sẻ định nghĩa profile giữa Make và Studio; EXTRA_DEFS thay thế cùng macro một lần, không tạo -D trùng. BH1750_ADDRESS cho phép override có kiểm tra địa chỉ.
- Kéo RX lên trong bản UART ON để tránh đầu vào trôi khi chưa gắn adapter; cần thử điện thật.
- Bảo vệ PORTD read-modify-write trong diagnostic trước ISR buzzer passive.
- Không ghi cờ lỗi EEPROM khi module EEPROM bị tắt và người dùng gọi Save/Load.
- Bổ sung test menu/nút, Base UART OFF, UART trong vùng khóa ngắt DHT; build/HEX/checksum/manifest mới. Driver DHT, LCD, EEPROM, TWI và thuật toán đã đúng được giữ lại.
- Thêm năm tài liệu bàn giao tiếng Việt. Ghi bất nhất nhãn PD2/PD3 của sơ đồ gốc và yêu cầu thông mạch trước thử buzzer; không sửa tài liệu gốc, fuse, clock, main hoặc nhánh bàn giao trước.

## 2026-10-08 — bản bàn giao trước

Firmware C hoàn chỉnh, 7 profile, GCC 14.2/5.4, test logic và simavr, source/driver/Studio/HEX/tài liệu. ZIP `ATmega16_Monitor_C.zip` vẫn là bản này và Base cũ bật UART; dùng gói BUILD_FIX cho bản mới.
