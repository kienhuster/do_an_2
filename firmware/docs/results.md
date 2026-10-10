# Kết quả build và kiểm thử hiện tại — 2026-10-10

[Báo cáo sửa lỗi và kết quả](../BUILD_FIX_REPORT.md), [bảng dung lượng/cấu hình](../BUILD_CONFIGURATIONS.md).

- Make: 8 profile × GCC 14.2/5.4, tất cả compile/link/objcopy đạt.
- XML Studio: 10 configuration × hai compiler, build từ cwd cấu hình và HEX khớp Make.
- Host ASan/UBSan: core 308, UI 20 assertions đạt.
- simavr: Base UART OFF 30 assertions trên cả hai compiler; Full 41 trên cả hai;
  Passive 41 và UART 31 trên GCC14.2.
- Các biến thể Full UART OFF, BH1750 0x5c và Minimal DHT OFF build đạt.
- HEX/ELF hiện tại trong releases/, SHA256SUMS.txt và BUILD_MANIFEST.json ghi nguồn gốc.
- GCC 5.4 Base UART OFF: Flash 8638, SRAM tĩnh 194, EEPROM 48 byte.
- GCC 14.2 Full: Flash 15002, SRAM tĩnh 508, EEPROM 48 byte. Stack chưa chứng minh worst case.

Chưa chạy GUI Studio Windows, PROGISP/ISP hoặc phần cứng thật. Logs trong ../test-evidence/.
Báo cáo ngày 2026-10-08 vẫn nằm trong lịch sử Git và ZIP bàn giao cũ; các số ở đó áp dụng Base UART ON cũ.
