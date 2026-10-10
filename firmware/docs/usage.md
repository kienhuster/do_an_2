# Sử dụng hệ thống

Base/Debug mặc định UART OFF. Các lệnh và CSV dưới đây dùng profile UART hoặc bản mở rộng có UART ON.

UART: 9600 baud, 8N1, TTL, không flow control. Gửi ASCII chữ hoa và kết thúc CR, LF hoặc
CRLF; tối đa 47 ký tự mỗi lệnh. Chờ `# OK` hoặc `# ERR ...` rồi gửi lệnh tiếp theo.
Các dòng phản hồi bắt đầu `#`; các dòng dữ liệu CSV bắt đầu bằng uptime dạng số.
Không gửi liên tục nhiều lệnh trong lúc ghi EEPROM. Bộ đệm và bộ nhận có phát hiện
overflow/framing error; dòng bị hỏng được loại bỏ, không áp dụng một lệnh bị cắt ngắn.

## LCD và bốn nút

UP/DOWN chuyển vòng chín trang:

| Trang | Nội dung |
|---|---|
| 0 | Nhiệt độ/độ ẩm hiện tại; A = alarm mask, ACK = mask đã xác nhận, E = lỗi |
| 1 | Nhiệt độ MIN/MAX/AVG và số mẫu N |
| 2 | Độ ẩm MIN/MAX/AVG và N |
| 3 | Xu hướng T/H, số mẫu trong cửa sổ 8 mẫu |
| 4 | Uptime ngày/giờ/phút/giây, nguyên nhân reset |
| 5 | ADC VR1, mã 0–1023 và điện áp ước tính theo AVCC=5 V |
| 6 | DS3231 ngày/giờ hoặc thông báo chưa có/tắt module |
| 7 | BH1750 lux hoặc thông báo lỗi/tắt module |
| 8 | Diagnostic: nút raw, LED test, lỗi |

SET vào menu từ các trang 0–7. UP/DOWN sửa mục đang hiển thị; SET đi tới mục kế tiếp:
TLOW → THIGH → HLOW → HHIGH → THYST → HHYST → CSVSEC → SOUND → Save & exit.
Nhấn SET ở trang Save để áp dụng và lưu EEPROM. ACK/BACK hủy các thay đổi đang sửa.
Trong menu, ngưỡng nhiệt/ẩm tăng giảm mỗi lần 1 °C/1 %RH; hysteresis tăng giảm 0.1 đơn vị.
Số hiển thị trong menu là đơn vị **phần mười**: 350 = 35.0 °C; 800 = 80.0 %RH.
Một lần nhấn tạo một sự kiện, không auto-repeat; chống dội ba mẫu ở chu kỳ 10 ms.

Ở trang 8, SET bật/tắt test LED; ACK xác nhận cảnh báo. Diagnostic riêng tự chạy LED
khi bật nguồn; có thể `DIAG OFF`/`DIAG ON` qua UART. Các trang LCD không xuất được
trong cấu hình `diagnostic` vì LCD đã tắt; dùng UART để xem số đo và cấu hình.

ACK luôn xác nhận các cảnh báo/lỗi đang có. Nó tắt âm thanh của **đợt hiện tại**, giữ
thông tin cảnh báo trên LCD/UART. Khi điều kiện đã hết rồi xuất hiện lại, hoặc có một
loại cảnh báo mới, âm thanh bật lại. SOUND=0 tắt âm thanh toàn bộ, không xóa cảnh báo.

## Ngưỡng, thống kê và xu hướng

Mặc định: nhiệt thấp 18.0 °C, nhiệt cao 35.0 °C; ẩm thấp 30.0 %, ẩm cao 80.0 %;
hysteresis nhiệt 1.0 °C, ẩm 3.0 %. Chạm ngưỡng (`<=` thấp, `>=` cao) kích hoạt cảnh báo.
Nhiệt cao chỉ hết khi nhiệt `<= THIGH-THYST`; nhiệt thấp hết khi `>= TLOW+THYST`.
Độ ẩm dùng quy tắc tương tự. Ngưỡng thấp phải nhỏ hơn ngưỡng cao và khoảng cách phải
lớn hơn hysteresis. Phạm vi cấu hình: T 0–50 °C, H 0–100 %, THYST 0.1–10 °C,
HHYST 0.1–20 %, CSVSEC 1–60 s, SOUND 0/1.

Alarm mask: 1=T thấp, 2=T cao, 4=H thấp, 8=H cao; nhiều điều kiện cộng bit.
LCD hiển thị mask/lỗi dạng hex; CSV dạng số thập phân.

DHT11 được đọc mỗi 2 s sau thời gian ổn định nguồn 2 s; chỉ mẫu đúng checksum và đúng
miền giá trị được đưa vào thống kê. Lần đọc lỗi làm dữ liệu hiện tại không hợp lệ;
ba lần lỗi liên tiếp đặt cờ DHT. Mẫu thành công tiếp theo xóa lỗi và tiếp tục thống kê.
MIN/MAX/AVG tính từ khởi động hoặc `RESETSTATS`; không lưu lịch sử đo vào EEPROM.
AVG sử dụng số nguyên phần mười, làm tròn xuống 0.1 đơn vị. Đến 1,000,000 mẫu hợp lệ
(khoảng 23.15 ngày ở chu kỳ 2 s), count/sum/AVG dừng tích lũy để tránh tràn số; MIN/MAX
và cửa sổ xu hướng vẫn cập nhật. Reset thống kê để mở một phiên đo mới.

Xu hướng so trung bình bốn mẫu gần nhất với bốn mẫu trước đó trong tám mẫu hợp lệ:
T tăng/giảm khi chênh ít nhất ±0.5 °C, H ít nhất ±2.0 %RH. +1/tăng, -1/giảm, 0/ổn định.
Chưa đủ tám mẫu thì CSV để trống; dữ liệu lỗi hiện tại cũng làm xu hướng CSV trống.
Cửa sổ là tám **mẫu hợp lệ**, không phải một cửa sổ thời gian cố định khi cảm biến mất kết nối.

## Lệnh UART

| Lệnh | Hành vi |
|---|---|
| `HELP` | Liệt kê lệnh |
| `STATUS` | Một hàng CSV trạng thái hiện tại |
| `CONFIG` | Đọc các giá trị cấu hình đang dùng trong RAM |
| `SET THIGH 350` | Đặt ngưỡng nhiệt cao 35.0 °C, chưa ghi EEPROM |
| `SET TLOW 180` / `SET HLOW 300` / `SET HHIGH 800` | Sửa ngưỡng khác |
| `SET THYST 10` / `SET HHYST 30` | Sửa hysteresis |
| `SET CSVSEC 2` / `SET SOUND 0` | Chu kỳ CSV / tắt âm |
| `SAVE` | Ghi cấu hình hai slot EEPROM và xác minh lại |
| `LOAD` | Nạp bản ghi EEPROM hợp lệ mới nhất |
| `DEFAULTS` | Khôi phục mặc định trong RAM; cần SAVE nếu muốn giữ qua reset |
| `STATS` / `RESETSTATS` | Đọc / xóa thống kê |
| `ACK` | Xác nhận đợt cảnh báo hiện tại |
| `CSV ON` / `CSV OFF` | Bật/tắt gửi định kỳ; ON gửi lại header |
| `PAGE 0` … `PAGE 8` | Chọn trang LCD, hủy menu đang sửa |
| `DIAG` | Đọc nút raw, nguyên nhân reset và trạng thái LED test |
| `DIAG ON` / `DIAG OFF` | Bật/tắt LED test trên chân không xung đột |
| `RTC 26-10-08 14:20:30 W=4` | Đặt DS3231 năm-tháng-ngày giờ:phút:giây, thứ 1–7 |
| `CLEARERR` | Xóa lỗi UART đã ghi nhận và cờ watchdog; lỗi thiết bị vẫn tự cập nhật |

Ví dụ RTC chỉ minh họa cú pháp, không phải giờ hiện tại. Chọn quy ước W=1 là thứ hai,
… W=7 chủ nhật và đặt nhất quán; driver kiểm tra miền giá trị nhưng không tự tính weekday
từ ngày. RTC không xử lý timezone/DST; đặt giờ UTC hoặc giờ địa phương theo nhu cầu,
CSV phản ánh đúng giờ được đặt và không thêm hậu tố timezone. Lệnh RTC sẽ bị từ chối khi
module tắt, NACK, ngày sai hoặc đọc xác minh thất bại; khi đặt thành công sẽ xóa OSF và
bật oscillator trên pin dự phòng (EOSC=0).

## CSV trên máy tính

Header:

```text
uptime_s,temp_c,humidity_pct,t_trend,h_trend,alarm,ack,errors,adc_raw,lux,rtc
```

Ví dụ minh họa một hệ thống có đủ module và số đo hợp lệ:

```text
25,25.0,60.0,0,0,0,0,0,511,1000,2026-10-08T14:20:30
```

Trường trống nghĩa là module tắt, chưa có mẫu, hoặc mẫu hiện tại không hợp lệ; không thay
bằng số đo cũ hay số 0 giả. RTC chỉ điền khi ngày giờ và OSF hợp lệ. Uptime bắt đầu lại
sau reset, độc lập RTC; clock millisecond chống wrap khoảng 49.7 ngày, uptime giây 32 bit.

Trên PC:

```bash
python -m pip install pyserial==3.5
python tools/capture_csv.py COM3 --output monitor.csv --seconds 120
# Linux: thay COM3 bằng /dev/ttyUSB0
```

Công cụ bỏ qua dòng `#`, giữ header và các dòng có 11 trường. Không mở cùng cổng serial
bằng terminal và capture tool đồng thời. Điện áp trên trang ADC chỉ là ước tính theo 5 V;
đo AVCC thực bằng đồng hồ nếu cần kiểm chuẩn.

## Lỗi hệ thống

| Bit / hex | Ý nghĩa và xử lý |
|---|---|
| 1 / 01 | DHT11 lỗi ít nhất ba lần; kiểm tra nguồn, PA1, pull-up, timing |
| 2 / 02 | DS3231 không trả lời, dữ liệu BCD/ngày sai hoặc OSF; kiểm tra bus/đặt giờ |
| 4 / 04 | BH1750 không trả lời/đọc thất bại; kiểm tra nguồn/mức logic/địa chỉ |
| 8 / 08 | ADC không hoàn tất trong timeout |
| 16 / 10 | UART framing/overrun/ring overflow/TX timeout; giảm tốc độ gửi lệnh |
| 32 / 20 | Chưa có cấu hình EEPROM hợp lệ hoặc ghi/đọc xác minh lỗi; SAVE nếu mới nạp |
| 64 / 40 | Lần khởi động do watchdog; kiểm tra nguồn và nguyên nhân treo, CLEARERR sau khi ghi nhận |

RTC được thử mỗi 1 s. BH1750 lỗi khởi tạo được thử lại mỗi 5 s, chờ conversion sau khi
kết nối thành công rồi đọc mỗi 2 s. I²C có timeout và recovery chín xung clock; một bus bị
giữ LOW liên tục vẫn báo lỗi và cần sửa wiring. Cảnh báo/lỗi thiết bị có thể phát buzzer
200 ms mỗi giây khi SOUND bật. Lỗi EEPROM và UART chỉ hiển thị, không tự phát buzzer.
