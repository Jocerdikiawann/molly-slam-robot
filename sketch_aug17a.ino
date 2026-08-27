#include <Arduino.h>

// --- Pin Driver TB6612FNG ---
// Motor Kanan (Channel A)
const int PIN_AIN1 = 18;
const int PIN_AIN2 = 19;
const int PIN_PWMA = 21;

// --- Pin Dual Encoder (JGA25-370) ---
// Encoder Kanan
const int PIN_ENC_R_A = 23; // Interrupt
const int PIN_ENC_R_B = 22;


// Motor Kiri (Channel B)
const int PIN_BIN1 = 25;
const int PIN_BIN2 = 33;
const int PIN_PWMB = 32;


// Encoder Kiri
const int PIN_ENC_L_A = 27; // Interrupt
const int PIN_ENC_L_B = 4;

// Stand By
const int PIN_STBY = 26;

// Variabel tick encoder (volatile karena diakses via ISR)
volatile long ticks_left = 0;
volatile long ticks_right = 0;

// --- ISR Encoder Kiri (Tersimpan di IRAM) ---
void IRAM_ATTR isrEncoderLeft() {
  if (digitalRead(PIN_ENC_L_B) > 0) {
    ticks_left++;
  } else {
    ticks_left--;
  }
}

// --- ISR Encoder Kanan ---
void IRAM_ATTR isrEncoderRight() {
  // Catatan: Arah putaran roda kanan berkebalikan secara mekanik terhadap bodi robot
  if (digitalRead(PIN_ENC_R_B) > 0) {
    ticks_right++;
  } else {
    ticks_right--;
  }
}

// --- Fungsi Pengendali Motor ---
// pwm_speed bernilai -255 s/d 255 (negatif = mundur, positif = maju)
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

void stopRobot() {
  // drive(0, 0);
  digitalWrite(PIN_STBY, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Inisialisasi Pin Motor
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_PWMA, OUTPUT);

  pinMode(PIN_STBY, OUTPUT);

  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT);

  // Inisialisasi Pin Encoder
  pinMode(PIN_ENC_L_A, INPUT_PULLUP);
  pinMode(PIN_ENC_L_B, INPUT_PULLUP);
  pinMode(PIN_ENC_R_A, INPUT_PULLUP);
  pinMode(PIN_ENC_R_B, INPUT_PULLUP);

  // Attach Interrupt ke Pin Fase A masing-masing encoder
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isrEncoderLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isrEncoderRight, RISING);

  digitalWrite(PIN_STBY, LOW);
  Serial.println("Differential Drive ESP32 Initialized!");
}

void loop() {
  digitalWrite(PIN_STBY, HIGH);
  // 1. Maju Bersama
  Serial.println("[Aksi] Maju");
  drive(180, 180);
  logTicks(20);

  // 2. Berhenti
  stopRobot();
  delay(500);

  // 3. Belok di Tempat (Pivot Kanan: Kiri Maju, Kanan Mundur)
  Serial.println("[Aksi] Belok Kanan");
  drive(160, -160);
  logTicks(15);

  // 4. Berhenti
  stopRobot();
  delay(500);

  // 5. Mundur Bersama
  Serial.println("[Aksi] Mundur");
  drive(-180, -180);
  logTicks(20);

  // 6. Berhenti
  stopRobot();
  delay(500);
}

// Utility untuk print encoder ke Serial Monitor secara periodik
void logTicks(int iterations) {
  for (int i = 0; i < iterations; i++) {
    Serial.print("L_Ticks: ");
    Serial.print(ticks_left);
    Serial.print("\t| R_Ticks: ");
    Serial.println(ticks_right);
    delay(100);
  }
}