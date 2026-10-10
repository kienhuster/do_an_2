# Build và nạp trên Windows 11 / Microchip Studio 7

## Chọn đúng bản đã sửa

Tải nhánh `fix/atmega16-studio-build-and-firmware` bằng **Code → Download ZIP**, giải nén hoàn toàn rồi mở `firmware/studio/MonitorATmega16.cproj`. Không mở project trực tiếp trong ZIP. Bản ZIP cũ `ATmega16_Monitor_C.zip` giữ lại để lưu lịch sử, không dùng để lấy bản sửa này.

HEX dựng sẵn dành cho toolchain AVR-GCC 5.4 ở `firmware/releases/windows-gcc54/`. Với kit hiện tại, chọn **ATmega16_DHT11_BASE_UART_OFF.hex**. File ELF cùng tên phục vụ debug; không nạp ELF bằng PROGISP. Các bản GCC 14.2 nằm ở `firmware/releases/<profile>/monitor.hex`; bản 5.4 ở `firmware/releases/legacy-<profile>/monitor.hex`. Không lấy HEX từ build cũ của Studio.

## Thiết lập project

1. Chọn device **ATmega16**, toolchain **AVR/GNU C Compiler** 5.4.0, không chọn C++.
2. Chọn **Debug** hoặc **Base**. Debug = Base, mặc định UART OFF. Release = Full và cần linh kiện mở rộng. Nếu chưa có tên cấu hình khác trong thanh chọn, dùng Build → Configuration Manager để tạo tên đúng như bảng BUILD_CONFIGURATIONS.md; PropertyGroup cùng tên đã có sẵn trong project. Khi tạo tên mới, chọn build project, kiểm tra lại Properties trước Rebuild.
3. Project Properties → Toolchain → AVR/GNU C Compiler → Directories: include phải là `..\..\include`. Trong Symbols, `F_CPU=8000000UL`; Base/Debug không có một `ENABLE_UART=1` cũ. Mặc định `ENABLE_UART=0` lấy từ `config.h`. Nếu đã chỉnh Properties trên Windows trước đây, dùng lại `.cproj` của nhánh sửa và đóng/mở project để loại cấu hình cũ.
4. Kiểm tra compiler GNU99, tối ưu `-Os`, warnings bật; linker `--gc-sections`. OutputPath mỗi cấu hình là `<Configuration>\`.
5. Build → Clean Solution rồi **Rebuild Solution**. Xem Output → Build để bảo đảm không còn thiếu `app.h`, `hal.h`, `core.h`.
6. Với Debug, compiler làm việc trong `firmware/studio/Debug/` và include `../../include` trỏ tới `firmware/include/`. Mục liệt kê source trong XML vẫn là `..\src\main.c`, tính từ nơi đặt `.cproj`; compiler-generated makefile sẽ dùng đường dẫn source tính lại theo thư mục build. Không sửa Makefile tự sinh để chữa đường dẫn lâu dài.
7. HEX mới của Studio: `firmware/studio/Debug/MonitorATmega16.hex` (hoặc `Base/`, `Full/`, ... theo cấu hình). ELF cùng thư mục. Đối chiếu dòng output linker/objcopy và thời gian cập nhật để biết chính xác file IDE vừa sinh; không giả định file trong `releases/` tự cập nhật từ GUI.

Nếu include vẫn là `-I"..\include"` trong Output, bạn đang dùng XML/cấu hình cũ. Đóng Studio, lấy `.cproj` mới, xóa **chỉ thư mục output tự sinh** của cấu hình rồi mở lại và Rebuild. Không xóa source/include. Khi đổi `config.h`, native toolchain sinh dependency; Rebuild là cách kiểm tra đầy đủ trên máy Windows. Cloud đã xác nhận header dependency thực tế qua GCC `-MMD -MP` và Make; cần xác nhận bộ Compiler.targets được cài trên máy Windows cũng rebuild đúng.

Tái tạo project bằng Python 3.9+: `py firmware/tools/generate_studio.py` từ gốc repo. Script này và Make dùng chung định nghĩa `firmware/tools/profiles.py`; không sửa XML rồi tái tạo nếu muốn giữ sửa đổi riêng.

## Xác nhận UART OFF

- Chọn Debug/Base; kiểm tra `ENABLE_UART` không được override thành 1 trong Symbols/OtherFlags.
- Có thể chạy PowerShell với đường dẫn GCC của Studio được thay cho đường dẫn ví dụ:

```powershell
& "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe" -mmcu=atmega16 -Ifirmware/include -dM -E -x c firmware/include/config.h | Select-String "ENABLE_UART"
Get-FileHash firmware/releases/windows-gcc54/ATmega16_DHT11_BASE_UART_OFF.hex -Algorithm SHA256
```

Chạy từ gốc repo. Lệnh preprocessing này xác minh mặc định Base; để kiểm tra Full/UART, thêm đúng các `-D` trong BUILD_CONFIGURATIONS.md hoặc manifest. Kết quả Base phải là `#define ENABLE_UART 0`. Với lệnh build IDE có định nghĩa riêng, phải kiểm tra **đúng flags của IDE**, không dùng lệnh Base để kết luận cho cấu hình khác. Base không có RX/TX rings, bộ phân tích lệnh, CSV hay ISR UART trong ELF; `uart_init` chỉ tắt UCSRB. Không tự phát `E:10` do lỗi UART ở bản này. `E:20` khi EEPROM chưa có record hợp lệ là tình trạng khác: dùng menu Save để lưu.

## Nạp bằng PROGISP / USB ISP

1. Ngắt nguồn khi đổi dây/jumper. Đối chiếu bảng `docs/wiring.md` và HARDWARE_TEST_GUIDE.md. Với LCD: tháo JP2; bình thường tháo JP1. DHT11 DATA tại PA1, VR1 tại PA0, B1–B4 tại PB0–PB3.
2. Kết nối đúng header J2 ISP (MOSI, MISO, SCK, RESET, VCC, GND). Chọn nguồn cấp phù hợp kit/programmer, tránh hai nguồn đấu trực tiếp với nhau. Không gắn tải vào PB5–PB7 khi nạp.
3. Mở PROGISP, chọn loại programmer thực sự đang dùng (ví dụ USBASP), chip **ATmega16**. Đọc signature, phải là **1E 94 03**. Nếu sai/không đọc được, dừng nạp và kiểm tra nguồn/ISP/driver; dùng SCK chậm nếu programmer hỗ trợ.
4. Đọc và sao lưu fuse/EEPROM nếu cần giữ cấu hình. Không tự động ghi fuse: thạch anh vẫn 8 MHz, `F_CPU` không thay fuse. Kiểm tra các ô Program Fuse / Program EEPROM đều tắt.
5. Load Flash file `ATmega16_DHT11_BASE_UART_OFF.hex`; Program FLASH và Verify FLASH. Các phiên bản PROGISP có tên nút khác nhau, chọn thao tác tương đương. Nếu dùng Auto, kiểm tra danh sách thao tác trước khi chạy. Chip erase có thể xóa EEPROM tùy EESAVE; sao lưu trước.
6. Không nạp `.eep` mặc định nếu muốn giữ cấu hình. Sau lần nạp mới/EEPROM trống, có thể thấy E:20; vào menu SET, đi đến Save & exit, SET để lưu rồi xác nhận lỗi đã hết.
7. Reset/cấp nguồn lại, chờ ít nhất khoảng 2.3 giây để có mẫu DHT đầu tiên. Thực hiện checklist phần cứng; ghi tên HEX, SHA256, ngày, wiring và kết quả.

## Giới hạn bằng chứng

Đã compile/link bằng AVR-GCC 5.4 và 14.2 trên Linux, kể cả tái hiện thư mục/flags từ XML của 10 cấu hình Studio. **Chưa chạy GUI Microchip Studio trên Windows**, chưa nạp bằng PROGISP/USB ISP và chưa thử kit thật. Các bước trên là hướng dẫn cần kiểm chứng trên máy/kit của bạn.
