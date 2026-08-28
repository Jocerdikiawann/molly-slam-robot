#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// --- Pin I2C Sensor GY-85 --- 
#define I2C_SCL 17
#define I2C_SDA 16 
#define ITG3205_ADDR 0x68

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
float gyro_z_offset = 0.0;

// --- Pin Driver TB6612FNG ---
const int PIN_AIN1 = 18;
const int PIN_AIN2 = 19;
const int PIN_PWMA = 21;

const int PIN_BIN1 = 25;
const int PIN_BIN2 = 33;
const int PIN_PWMB = 32;
const int PIN_STBY = 26;

// --- Pin Dual Encoder (JGA25-370) ---
const int PIN_ENC_R_A = 23; 
const int PIN_ENC_R_B = 22;
const int PIN_ENC_L_A = 27; 
const int PIN_ENC_L_B = 4;

volatile long ticks_left = 0;
volatile long ticks_right = 0;

// Variabel Timer untuk pengiriman data
unsigned long last_time = 0;

void IRAM_ATTR isrEncoderLeft() {
  if (digitalRead(PIN_ENC_L_B) > 0) ticks_left++;
  else ticks_left--;
}

void IRAM_ATTR isrEncoderRight() {
  if (digitalRead(PIN_ENC_R_B) > 0) ticks_right++;
  else ticks_right--;
}

void setMotorRight(int pwm_speed) {
  pwm_speed = constrain(pwm_speed, -255, 255);
  if (pwm_speed > 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, pwm_speed);
  } else if (pwm_speed < 0) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
    analogWrite(PIN_PWMA, abs(pwm_speed));
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    analogWrite(PIN_PWMA, 0);
  }
}

void setMotorLeft(int pwm_speed) {
  pwm_speed = constrain(pwm_speed, -255, 255);
  if (pwm_speed > 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, pwm_speed);
  } else if (pwm_speed < 0) {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
    analogWrite(PIN_PWMB, abs(pwm_speed));
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    analogWrite(PIN_PWMB, 0);
  }
}

void drive(int left_speed, int right_speed) {
  setMotorLeft(left_speed);
  setMotorRight(right_speed);
}

void initITG3205() {
  Wire.beginTransmission(ITG3205_ADDR);
  Wire.write(0x16); Wire.write(0x18); 
  Wire.endTransmission();

  Wire.beginTransmission(ITG3205_ADDR);
  Wire.write(0x3E); Wire.write(0x01); 
  Wire.endTransmission();
}

float readGyroZ() {
  Wire.beginTransmission(ITG3205_ADDR);
  Wire.write(0x21); 
  Wire.endTransmission(false);
  Wire.requestFrom(ITG3205_ADDR, 2, true);

  if(Wire.available() >= 2) {
    int16_t raw_z = (Wire.read() << 8) | Wire.read();
    float deg_per_sec = (float)raw_z / 14.375;
    return (deg_per_sec * (PI / 180.0)) - gyro_z_offset; // Rad/s
  }
  return 0.0;
}

void calibrateGyroZ() {
  float sum = 0.0;
  int samples = 200;
  for(int i = 0; i < samples; ++i) {
    Wire.beginTransmission(ITG3205_ADDR);
    Wire.write(0x21);
    Wire.endTransmission(false);
    Wire.requestFrom(ITG3205_ADDR, 2, true);
    if(Wire.available() >= 2) {
      int16_t raw_z = (Wire.read() << 8) | Wire.read();
      sum += ((float) raw_z / 14.375) * (PI / 180.0);
    }
    delay(10);
  }
  gyro_z_offset = sum / samples;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  if(!accel.begin()) {
    while(1) delay(10); // Halt jika accel gagal
  }
  accel.setRange(ADXL345_RANGE_4_G);

  initITG3205();
  calibrateGyroZ();

  pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT); pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT); pinMode(PIN_PWMB, OUTPUT);
  pinMode(PIN_STBY, OUTPUT); digitalWrite(PIN_STBY, HIGH); // Standby selalu ON

  pinMode(PIN_ENC_L_A, INPUT_PULLUP); pinMode(PIN_ENC_L_B, INPUT_PULLUP);
  pinMode(PIN_ENC_R_A, INPUT_PULLUP); pinMode(PIN_ENC_R_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isrEncoderLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isrEncoderRight, RISING);
}

void loop() {
  // ==========================================
  // 1. TERIMA PERINTAH MOTOR DARI RASPBERRY PI
  // ==========================================
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    
    // Format yang diharapkan: CMD,PWM_KIRI,PWM_KANAN
    // Contoh untuk belok: CMD,150,-150
    if (data.startsWith("CMD,")) {
      int pwm_l = 0, pwm_r = 0;
      sscanf(data.c_str(), "CMD,%d,%d", &pwm_l, &pwm_r);
      drive(pwm_l, pwm_r);
    }
  }

  // ==========================================
  // 2. KIRIM DATA SENSOR KE RASPBERRY PI (50 Hz)
  // ==========================================
  unsigned long current_time = millis();
  if (current_time - last_time >= 20) {
    last_time = current_time;

    // Ambil data IMU
    sensors_event_t event;
    accel.getEvent(&event);
    float ax = event.acceleration.x;
    float ay = event.acceleration.y;
    float az = event.acceleration.z;
    float gz = readGyroZ();

    // Matikan interrupt sejenak agar pembacaan tick stabil
    noInterrupts();
    long current_ticks_left = ticks_left;
    long current_ticks_right = ticks_right;
    interrupts();

    // Format output: DATA,ax,ay,az,gx,gy,gz,ticks_left,ticks_right
    Serial.print("DATA,");
    Serial.print(ax, 3); Serial.print(",");
    Serial.print(ay, 3); Serial.print(",");
    Serial.print(az, 3); Serial.print(",");
    Serial.print(0.0, 3); Serial.print(","); // gx tidak dibaca
    Serial.print(0.0, 3); Serial.print(","); // gy tidak dibaca
    Serial.print(gz, 3); Serial.print(",");
    Serial.print(current_ticks_left); Serial.print(",");
    Serial.println(current_ticks_right);
  }
}
