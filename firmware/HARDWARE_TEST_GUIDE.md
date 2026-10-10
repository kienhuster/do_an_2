# Checklist kiểm thử Kit AVR thật

Tất cả bài thử sau đây **chưa thực hiện trên phần cứng thật**. Ghi: ngày/người thử, phiên bản PCB, signature/fuse chỉ đọc, compiler/profile, SHA256 HEX, wiring, kết quả và ảnh/log nếu có. Dùng bản Base UART OFF cho linh kiện hiện có.

## Đối chiếu trước cấp nguồn

Đã đọc sơ đồ nguyên lý trang 30 của `Merged - Tai lieu huong dan.pdf`, hình layout và bảng linh kiện trong `So do mach Kit.docx`. Cách đấu nền phù hợp: VR1→PA0, DATA DHT11→PA1, PB1/B1→PB0 UP, PB2/B2→PB1 DOWN, PB3/B3→PB2 SET, PB4/B4→PB3 ACK/BACK. PB5 trên PCB là nút RESET, không phải GPIO PB5. R3 kéo lên PORTB, firmware thêm pull-up bốn nút. MOSI/MISO/SCK dùng PB5/PB6/PB7; firmware không xuất lên các chân ISP này.

LCD bus PC4–PC7 và PD5 R/W, PD6 RS, PD7 E. JP2 cấp anode chung LED 7 đoạn PORTC, cần tháo khi dùng LCD/I²C. JP1 cấp dãy LED đơn PORTD, tháo khi vận hành bình thường, chỉ lắp cho bài diagnostic đã đối chiếu. Firmware tắt JTAG bằng hai lần ghi MCUCSR liên tiếp khi cli; không đổi fuse.

**Điểm cần kiểm tra thông mạch:** sơ đồ PDF in nhãn PD3 tại chân IC2 số 16 (chân này theo datasheet ATmega16 là PD2), và PD3 tại số 17. Ký hiệu header/module có thể không phản ánh đúng PCB thực tế. Buzzer hiện tắt trong Base; không cắm buzzer theo nhãn này trước khi đo thông mạch đến **DIP-40 chân 16 / PD2** và xác nhận không nối nhầm/chập với chân 17. Đây là bất nhất trong tài liệu, chưa có bằng chứng rằng PCB thực bị chập. I²C lấy PC0/chân 22 và PC1/chân 23; J5 trên sơ đồ đi theo thứ tự PC7…PC0 (PC0 ở J5/8, PC1 ở J5/7), không suy số header từ số GPIO.

Kiểm tra nguồn 5 V/AVCC/GND, tụ và thạch anh 8 MHz thực. Không ghi fuse thử nghiệm để chữa lỗi firmware. Kit có contrast LCD qua đường V0 riêng; VR1 vẫn dành ADC. Xác nhận vị trí chân/VCC module DHT11 từ nhãn/datasheet của module thực, không suy theo vỏ 3/4 chân.

## Các bài thử Base / Debug

| Bài thử | Thao tác | Tiêu chí cần quan sát |
|---|---|---|
| Khởi động | Reset 10 lần, không nhấn nút | Không tự vào menu/chuyển trang; sau thời gian ổn định có nhiệt/ẩm; UART OFF không E:10 |
| LCD | UP/DOWN qua chín trang, chỉnh contrast | Hai dòng rõ, không rác/ký tự cũ; RTC/Light disabled khi chưa có module |
| B1/UP, B2/DOWN | Nhấn/giữ/thử dội từng nút | Một sự kiện mỗi lần nhấn, không auto-repeat; chuyển trang vòng đúng |
| B3/SET | Từ trang 0 vào menu, sửa TLOW/THIGH/... | Giá trị theo phần mười; ngưỡng bất hợp lệ bị từ chối |
| B4/ACK/BACK | Sửa rồi BACK; vào lại | Thay đổi bị hủy, không ghi EEPROM; cảnh báo hiện có được ACK |
| Save | SET qua menu tới Save & exit rồi SET | Áp dụng và ghi EEPROM, E:20 hết nếu ghi thành công |
| EEPROM | Save, reset, tắt/bật nguồn | Ngưỡng đã lưu còn nguyên; thử mất nguồn giữa Save chỉ trong kế hoạch có kiểm soát, giữ bản cũ hoặc mới hợp lệ |
| DHT11 | Đối chiếu nhiệt/ẩm kế, theo dõi 10 phút | Mẫu hợp checksum, thay đổi hợp lý trong sai số sensor |
| Mất sensor | Ngắt nguồn rồi tháo DATA/nguồn DHT, cấp lại | No data; sau 3 lần lỗi E:01; không treo. Ngắt nguồn để gắn lại, lỗi tự hết khi có mẫu |
| Alarm | Đặt ngưỡng để thử từng loại thấp/cao T/H | Alarm bits 1/2/4/8 đúng, không nhấp nháy ở vùng hysteresis; ACK giữ mask; điều kiện hết rồi trở lại được tái báo |
| Thống kê | Chạy tập đo tăng/giảm/ổn định có đối chiếu | MIN/MAX/AVG hợp lý, xu hướng chỉ đánh giá khi đủ 8 mẫu hợp lệ |
| ADC VR1 | Chuyển trang 5, xoay min/mid/max, đo wiper | ADC gần 0/512/1023 tùy VR1; mV theo AVCC thực, bản hiển thị giả định 5 V |
| Uptime | So đồng hồ 1 giờ, DHT và thao tác nút đồng thời | Không mất tick do khóa ngắt ngắn; sai số theo thạch anh thật |
| Watchdog | Quan sát reset bình thường; treo chỉ với bản test riêng | WDRF/E:40 nếu watchdog reset, cấu hình EEPROM còn; không chèn treo vào HEX bàn giao |
| Chạy lâu | Base chạy nhiều giờ, lặp menu/Save vừa phải | Không reset bất thường, rác LCD, lỗi nguồn hoặc treo |

## Bài thử mở rộng sau khi xác minh đấu nối

- UART profile: nối USB-UART **TTL** đúng điện áp, chung GND, PD0 RX ← TX adapter, PD1 TX → RX adapter; 9600 8N1. Thử HELP/STATUS/SET/SAVE/CSV; dữ liệu 11 cột. Gửi lệnh trong lúc DHT đo, framing/line overflow phải loại lệnh hỏng. RX pull-up mới cần kiểm chứng khi adapter tháo; không dùng RS232 trực tiếp.
- Full/Passive: chỉ thử khi đã xác minh PD2 chân 16, transistor/nguồn buzzer và module I²C. Active phát chirp; passive dùng Timer0 CTC, sóng 2 kHz cần logic analyzer/oscilloscope. Thử diagnostic trên chân trống đồng thời buzzer để kiểm tra PORTD không bị tranh ghi.
- DS3231: PC0 SCL/PC1 SDA, địa chỉ 0x68, điện áp/pull-up hợp module. Đặt giờ qua UART, kiểm OSF, rút nguồn chính khi có pin phù hợp; so thời gian thực. Mô phỏng không xác nhận pin dự phòng/sai số clock.
- BH1750: xác nhận nguồn và bộ chuyển mức trước đấu. Chip trần không chịu 5 V; dùng breakout 5 V đã xác nhận hoặc nguồn 3.3 V + chuyển mức I²C. ADDR 0→0x23, 1→0x5c. Thử tối/sáng, hồi phục sau mất module.
- Diagnostic LED7: tháo LCD và I²C trước, đối chiếu sơ đồ rồi lắp JP2; JP1 cho LED đơn nếu muốn. Dùng profile Diagnostic, UART vẫn ON. Quan sát 0–9, 300 ms mỗi số; xác nhận ISP và các nút không bị chiếm.

Không hot-plug module chỉ để tái hiện mô phỏng nếu phần cứng không hỗ trợ. Không đánh dấu đạt phần cứng dựa trên kết quả compile/test cloud. Danh sách rộng hơn và giới hạn simulator: `docs/validation.md`.
