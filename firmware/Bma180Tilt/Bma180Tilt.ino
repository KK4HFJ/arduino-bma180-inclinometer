// Bma180Tilt - BMA180 3-axis accelerometer: g on each axis plus pitch and roll angles
// Steve Spence (KK4HFJ) - https://kk4hfj.com/
// 2026 cleanup of the Instructable "3 Axis Arduino Accelerometer / Inclinometer".
// Register sequence from the geeetech.com BMA180 sample; see README.md for the review notes.
//
//   BMA180 breakout: VDD 3.3 V, GND, SCK -> SCL (A5), SDI -> SDA (A4), CSB -> 3.3 V (I2C mode),
//   SDO -> GND (address 0x40) or 3.3 V (0x41)

#include <Wire.h>

const uint8_t BMA180 = 0x40;
const uint8_t REG_DATA = 0x02, REG_CTRL0 = 0x0D, REG_RESET = 0x10, REG_BW = 0x20, REG_RANGE = 0x35, REG_ID = 0x00;
const float LSB_PER_G = 4096.0;   // +/-2 g range, 14 bit: 0.25 mg per count

// Zero offsets in counts. Lay the board flat and still, read the raw values, then set
// OFF_X/OFF_Y to the negative of X/Y and OFF_Z to (4096 - Z).
const int OFF_X = 0, OFF_Y = 0, OFF_Z = 0;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BMA180);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readRegs(uint8_t reg, uint8_t n, uint8_t *buf) {
  Wire.beginTransmission(BMA180);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;   // repeated start, then read
  uint8_t got = Wire.requestFrom(BMA180, n);
  for (uint8_t i = 0; i < got; i++) buf[i] = Wire.read();
  return got;
}

bool initBma180() {
  uint8_t id = 0;
  if (readRegs(REG_ID, 1, &id) != 1 || id != 0x03) return false;   // chip_id is 3
  writeReg(REG_RESET, 0xB6);   // soft reset
  delay(10);
  writeReg(REG_CTRL0, 0x10);   // ee_w = 1: unlock the image registers so BW and RANGE can be written
  uint8_t v;
  readRegs(REG_BW, 1, &v);    writeReg(REG_BW, v & 0x0F);             // bandwidth 10 Hz
  readRegs(REG_RANGE, 1, &v); writeReg(REG_RANGE, (v & 0xF1) | 0x04);  // range +/-2 g
  return true;
}

// Each axis: LSB register holds bits 5..0 in [7:2] plus a new_data flag in bit 0; MSB holds bits 13..6.
// Build the signed 16-bit word and shift right 2 to get the signed 14-bit count.
int16_t axis(const uint8_t *p) { return (int16_t)((uint16_t)p[1] << 8 | p[0]) >> 2; }

void setup() {
  Serial.begin(9600);
  Wire.begin();
  while (!initBma180()) {
    Serial.println(F("BMA180 not found at 0x40 - check wiring / SDO pin"));
    delay(2000);
  }
  Serial.println(F("x_g,y_g,z_g,pitch_deg,roll_deg"));
}

void loop() {
  uint8_t d[6];
  if (readRegs(REG_DATA, 6, d) == 6) {
    float x = (axis(d) + OFF_X) / LSB_PER_G;
    float y = (axis(d + 2) + OFF_Y) / LSB_PER_G;
    float z = (axis(d + 4) + OFF_Z) / LSB_PER_G;
    // Tilt angles from gravity. Valid when the vehicle isn't accelerating hard.
    float pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / PI;   // front-to-back
    float roll = atan2(y, z) * 180.0 / PI;                       // side-to-side
    Serial.print(x, 3); Serial.print(',');
    Serial.print(y, 3); Serial.print(',');
    Serial.print(z, 3); Serial.print(',');
    Serial.print(pitch, 1); Serial.print(',');
    Serial.println(roll, 1);
  } else {
    Serial.println(F("read failed"));
  }
  delay(300);
}
