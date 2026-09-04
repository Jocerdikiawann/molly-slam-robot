## Molly SLAM 3D

A personal repository dedicated to my learning journey in robotics using **ROS2 Jazzy**. 

After years of diving deep into software engineering down to the low-level programming stage, I wanted to taste something new just for fun. Built upon my existing programming fundamentals, this project is my sandbox to explore and understand the intricate mutual bridge between electronic hardware and software logic.

### Software :
- ROS 2 Jazzy
- Ubuntu 24.0
- Gazebo (for Simulation)

### Component :
- RPLIDAR A1M8
- Breadboard (5 unit)
- Kabel AWG 22 (15 Meter)
- Kabel AWG 24 (10 Meter)
- PCB (5 Unit)
- Motor Driver TB6612FNG
- DC Motor JGA25-370 (2 Unit)
- Raspberry 4B
- Raspberry Cam Module 3
- Wago Connector 221 (5 Buah)
- Screw Terminal (150 Buah)
- Laptop Advan AI Gen Ultra (Untuk Computer Vision Raspberry ga bakal kuat, nabung beli jetson orin nano)
- Baterai holder 2 Slot
- Lion baterai 18650 (2 Buah)
- IMU GY-85

### Controller PIN
| Kabel | Pin GPIO | Description | Source | 
| -------- | -------- | -------- | -------- |
| STBY |  26 | Stand By (Untuk On/Off motor) | Motor Driver | 

#### Motor Kiri:
| Kabel | Pin GPIO | Description | Source | 
| -------- | -------- | -------- | -------- |
| Kuning | 27 | Interupt | DC Motor |
| Hijau | 14 | - | DC Motor|
| PWMB | 32 | Control | Motor Driver |
| BI2 | 33 | Control | Motor Driver |
| BI1 | 25 | Control | Motor Driver|

#### Motor Kanan:
| Kabel | Pin GPIO | Description | Source | 
| -------- | -------- | -------- | -------- |
| Kuning | 23 | Interupt | DC Motor |
| Hijau | 22 | Phase 2 | DC Motor|
| AI1 | 18 | Control | Motor Driver |
| AI2 | 19 | Control | Motor Driver |
| PWMA | 21 | Control | Motor Driver|

#### IMU:
| Imu Source | Esp32 Pin | Description | 
| -------- | -------- | -------- |
| VCC | 5V (Wago 221) | Power |
| GND | GND (Wago 221) | Power | 
| SCL | 17 | Jalur Data I2C |
| SDA | 16 | Jalur Data I2C |

### Driver
| TB6612FNG | Motor | Description |
| -------- | -------- | -------- |
| AO1 | putih |  motor kanan |
| AO2 | merah | motor kanan |
| BO1 | merah |  motor kiri (mirrored) |
| BO2 | putih | motor kiri (mirrored) |

## Note
| Kategori | PIN | Aturan Penggunaan |
| -------- | -------- | -------- |
| Input-Only (GPI) | 34, 35, 36 (VP), 39 (VN) |	Hanya bisa membaca sinyal (INPUT). Tidak bisa OUTPUT atau internal PULLUP/PULLDOWN. Aman untuk sensor/encoder yang punya resistor sendiri. |
| Strapping Pins (Booting) |	0, 2, 5, 12, 15 |	Menentukan mode boot ESP32 saat pertama dinyalakan. Hindari memasang beban yang menarik pin ini ke HIGH/LOW saat startup karena bisa menyebabkan ESP32 gagal menyala / stuck di flash mode. |
| Flash Memory Internal |	6, 7, 8, 9, 10, 11	| Terhubung langsung ke chip memory flash onboard. Mengakses pin ini akan membuat program langsung crash/reboot. |
| General Purpose (Bebas)	| 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33	| Pin standar yang fleksibel untuk PWM, SPI, I2C, UART, atau Digital I/O. |
