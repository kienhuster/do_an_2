# Báo cáo sửa lỗi build và firmware

Ngày xác minh: 2026-10-10 (giờ người dùng Asia/Bangkok). Nhánh `fix/atmega16-studio-build-and-firmware`, dựa trên main đã merge bản bàn giao (1e070ae). Đọc mọi `.c/.h`, test, tool, Makefile, XML Studio; đối chiếu sơ đồ PDF trang 30 và layout DOCX. Mã nguồn gốc, PDF/DOCX, bản backup và ZIP bàn giao cũ được giữ nguyên.

## Lỗi / thay đổi có lý do kỹ thuật

| Vấn đề | Nguyên nhân và ảnh hưởng | Cách sửa | Bằng chứng |
|---|---|---|---|
| Studio thiếu hal.h/app.h/core.h | Include ../include tính từ studio/Debug trỏ nhầm studio/include; các đường dẫn item XML được tính từ nơi khác | Include ../../include trong mọi cấu hình, OutputPath là thư mục cấu hình; sửa generator | 10 cấu hình compile/link ở cwd studio/<name> trên GCC 5.4 và 14.2; HEX khớp Make |
| Base/Debug phát UART dù chưa dùng | ENABLE_UART mặc định 1, UART RX có thể nhận lỗi khi không có adapter; parser/CSV chiếm tài nguyên | Mặc định 0, UART profile riêng; #if loại tác vụ/parser/CSV/buffer/ISR, UCSRB=0 lúc init ở bản OFF | dM xác nhận UART=0, nm không có parser/rings/ISR UART mạnh, sim UART OFF không RX/TX và không E:10 |
| Cấu hình Make/Studio dễ lệch và override tạo macro trùng | Hai bảng flags riêng; EXTRA_DEFS cũ nối thêm cùng macro profile | Dùng chung tools/profiles.py, hợp nhất theo tên trước tạo flags | XML/Make preprocessing và 20 cặp HEX so byte; thử Full UART OFF không redefinition |
| RX UART trôi khi bỏ adapter | Init cũ không bật pull-up RX | Set DDR/PORT RX phù hợp, idle HIGH khi UART ON | Kiểm source/compile/mô phỏng nhận framing lỗi và lệnh; điện áp/nhiễu thực chưa đo |
| Diagnostic tranh ghi PD2 với buzzer passive | PORTD read-modify-write có thể bị Timer0 ISR xen giữa và ghi đè trạng thái PD2 | Atomic trên các thao tác PORTD theo mask; không chiếm chân module đang bật | Rà soát assembly/source; Full/Passive sim đạt, cần đo tone/diagnostic đồng thời trên kit |
| BH1750_ADDR không override được đúng | #define không có #ifndef gây redefinition khi -D địa chỉ khác | #ifndef và kiểm tra chỉ 0x23/0x5c | Build với ADDR HIGH 0x5c đạt |
| Module EEPROM OFF vẫn tạo lỗi EEPROM khi Save/Load | app_save/app_load gọi stub không khả dụng rồi đặt ERR_EEPROM | Guard ENABLE_EEPROM để không đưa lỗi của module tắt vào trạng thái | Preprocessing/compile Minimal; chức năng EEPROM ON vẫn qua tests Save/Load/reset |
| Nguy cơ chọn nhầm HEX cũ | Tên monitor.hex giống nhau và ZIP cũ Base bật UART | Snapshot mới cho 16 bản, alias tên rõ, manifest flags/macros/source hashes và SHA256 | HEX mới khác Base cũ; objcopy, checksum Intel HEX, manifest và snapshot được đối chiếu |

Không sửa driver/thuật toán đã đúng chỉ để viết lại: giữ DHT11, LCD 4 bit, CRC/dual-slot EEPROM, thống kê/xu hướng, hysteresis và timeout TWI. Không thêm motor/PWM động cơ, không đổi F_CPU, không ghi fuse.

## Rà soát timing, ngắt và tài nguyên

Timer1 chạy free-running 1 MHz (prescaler 8) với overflow 65.536 ms để tính uptime; compare tăng OCR1A 10000 để đọc nút mỗi 10 ms. DHT low start 20 ms vẫn cho phép ngắt, chỉ khóa ngắt khi nhận frame khoảng 4 ms, Timer2 1 MHz đo độ rộng xung 27/70 µs và timeout 100 µs. Critical section này ngắn hơn một chu kỳ debounce và overflow Timer1, nên cờ ngắt pending được xử lý sau khi phục hồi SREG; clock_ms bù overflow pending, không giả định mỗi interrupt là 1 ms. UART ON có uart_rx_poll trong vòng chờ xung, lấy dữ liệu khỏi UDR khi CLI; UART OFF không có thao tác RX trong driver. UART TX chậm lại trong critical section, không cần giả dữ liệu. Bài thử mới gửi CONFIG trong chính critical section DHT và kiểm tra cả lệnh, mẫu và error mask. Mô phỏng không thay thế đo timing trên 8 MHz thật.

EEPROM dùng CRC16 và commit byte cuối, giữ slot cũ; không tự ghi mỗi mẫu. ADC timeout 5 ms; watchdog 1 s, reset_cause .noinit và wdt_disable trong .init3. LCD chỉ viết GPIO nên không tự phát hiện dây hở; buzzer cũng không thể tự xác nhận có tải. DS3231 dùng repeated START, BCD/date/OSF validation; BH1750 gửi lệnh thực, đợi conversion rồi đọc; TWI có timeout và 9 xung recovery. Bus SCL bị giữ LOW vẫn phải xử lý phần cứng, không có đảm bảo recovery khi dây chập.

Base mới: Flash 8690/8638 byte (GCC 14.2/5.4), SRAM tĩnh 194, EEPROM 48; UART cũ Base SRAM 490. Full: Flash 15002/14864, SRAM 508, EEPROM 48; Passive 15052/14924. Mọi cấu hình trong giới hạn chip và chừa >=256 byte stack tĩnh. Bảng đầy đủ ở BUILD_CONFIGURATIONS.md. Watermark Full >=336 byte (GCC14.2), >=338 (GCC5.4), UART >=378 (GCC14.2) trên các đường test; chưa phải worst-case stack proof.

## Kiểm thử thực sự đã chạy

| Nhóm | Kết quả | Phạm vi |
|---|---|---|
| Host core ASan/UBSan | 308 assertions / 6 nhóm đạt | Settings/narrowing/parser, 4 alarm/hysteresis/ACK, stats/trend/limit, CRC/torn-write model, RTC calendar/BCD, DHT codec/time wrap |
| Host UI ASan/UBSan | 20 assertions đạt | 4 phím, trang, edit/cancel/save, giới hạn, ACK, diagnostic, hiển thị ADC |
| Base UART OFF GCC14.2 | 30 assertions simavr đạt | DHT/LCD, dội/giữ/4 nút, menu hủy/lưu, EEPROM qua watchdog, ACK/rearm, sensor lỗi/khôi phục, ADC/uptime, UART tắt, ISP GPIO/JTAG |
| Base UART OFF GCC5.4 | 30 assertions simavr đạt | Cùng fixture Base |
| UART GCC14.2 | 31 assertions simavr đạt | UART/CSV/command/errors, DHT/ADC/LCD, EEPROM, watchdog, DHT+UART chồng thời gian |
| Full GCC14.2 và GCC5.4 | 41 assertions mỗi bản đạt | Thêm RTC read/write/OSF/EOSC, BH1750/lux, buzzer, thiếu/khôi phục I²C |
| Passive GCC14.2 | 41 assertions đạt | Full + Timer0 buzzer passive; chưa đo âm/tần số trên mạch thật |
| Make matrix | 8 profiles × 2 compiler đều build/link/objcopy đạt | -Wall -Wextra -Werror, GNU99, -Os, kiểm Flash/SRAM/EEPROM |
| Studio XML cwd | 10 configurations × 2 compiler đạt | Debug/Release/Base/UART/Full/RTC/Light/Passive/Diagnostic/Minimal; XML item paths khác compiler include; 20 HEX khớp Make byte-for-byte |
| Dependency | config.h đổi timestamp làm 12 object Base được compile lại; build lần sau không compile lại | Make thực, dependency .d; Studio compiler check cũng sinh .d có config.h, GUI còn phải kiểm tra |
| Các biến thể | Full UART OFF, BH1750 address 0x5c, Minimal DHT OFF build đạt | Giữ separate output, không dùng flag override trùng |

Đã dùng simavr upstream v1.8. Fixture vẫn chỉ bổ sung metadata pool cho pseudo-IRQ watchdog reset-only của simulator như bản trước; không sửa firmware để tránh lỗi test. Simulator còn mặc định TXEN=1 để debug printf; firmware OFF nay chủ động UCSRB=0 nên trạng thái OFF được kiểm thực tế trong ELF. Mô hình DHT/LCD/ADC/UART/TWI nằm duy nhất trong tests, không trong firmware. Lệnh treo test watchdog chỉ chèn vào Flash trong simulator; không xuất vào HEX.

Logs thô và XML verification.json có trong gói bàn giao dưới firmware/test-evidence/. Source/flags/macros/size/hash: releases/BUILD_MANIFEST.json. Native make matrix và XML build đều dùng compiler Linux; **không phải đã chạy Microchip Studio GUI trên Windows**. Không có kết quả nạp PROGISP, logic analyzer, oscilloscope, kit thật, sensor thật, pin RTC hay USB-UART thật.

## Đấu nối và vấn đề chưa thể xác minh

JP2 phải tháo khi LCD/I²C; JP1 tháo cho vận hành thường. Sơ đồ có nhãn PD3 lặp tại chân MCU 16 và 17 trong khi chân 16 là PD2 theo datasheet; cần đo thông mạch thực trước buzzer. Không kết luận PCB bị chập từ nhãn tài liệu, không thay GPIO đang đúng. Base không dùng buzzer, nên không cần thay mạch hiện tại. Bảng đầy đủ và thao tác thử có kiểm soát ở HARDWARE_TEST_GUIDE.md và docs/wiring.md.

Mỗi bản mới đã viết code/build; logic hoặc đường mô phỏng chỉ được đánh dấu theo bảng. Chưa có bản nào được kiểm thử trên kit thật. RTC/Light/Diagnostic/Minimal có build và logic driver liên quan được thử trong các profile khác, chưa chạy riêng simulator toàn bộ từng profile này.
