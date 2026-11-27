# 3-Axis Arduino Accelerometer / Inclinometer (BMA180)

BMA180 accelerometer read over I2C: g on X, Y and Z, plus pitch and roll angles — the start of an off-road tilt/roll gauge.

Instructable: [3 Axis Arduino Accelerometer / Inclinometer (Tilt / Roll / Yaw)](https://www.instructables.com/An-Arduino-Accelerometer/)
Author: Steve Spence, KK4HFJ — <https://kk4hfj.com/>. Register sequence from Geeetech's BMA180 sample code.

## Contents

| Path | What it is |
|---|---|
| `firmware/Bma180Tilt/Bma180Tilt.ino` | Fixed sketch: correct 14-bit decoding, pitch/roll angles, CSV output. Compiles in CI. |
| `legacy/bma180_geeetech.ino` | Step 2 sketch as published (Geeetech sample), unchanged. |

## Hardware

| BMA180 pin | Connect to |
|---|---|
| VDD, VDDIO | 3.3 V |
| GND | GND |
| SCK | SCL (Uno A5) |
| SDI | SDA (Uno A4) |
| CSB | 3.3 V (selects I2C) |
| SDO | GND → address 0x40 (3.3 V → 0x41) |

The BMA180 was discontinued by Bosch around 2012 and genuine parts are hard to find. For a new build use a BMI270, LSM6DSO, MPU-6050 or ADXL345 breakout — the pitch/roll maths in this sketch is the same for all of them.

## Review findings (published code)

| # | Issue | Effect | Fix |
|---|---|---|---|
| 1 | `#include <wire.h>` (lower case) | Compile error on Linux/macOS | `<Wire.h>` |
| 2 | `byte result[5]` read with 6 bytes | Buffer overflow: the Z MSB is written past the array | `uint8_t d[6]` |
| 3 | `result[0] \| result[1]<<8>>2` | Precedence makes this `lsb \| (msb << 6)`: the LSB's status bit and unshifted bits are mixed in, and the sign is lost, so negative g reads as +2 to +4 g | `(int16_t)(msb<<8 \| lsb) >> 2` |
| 4 | `Wire.beginTransmission()` around `requestFrom()` | Not needed, sends a stray empty write | Repeated-start read |
| 5 | No delay after soft reset | Writes right after reset can be lost | 10 ms delay, chip ID check |
| 6 | Hard-coded offsets 31/47/−23 | Those belong to one particular board | Zero offsets with a how-to in the sketch |
| 7 | "Yaw (pivoting as in a skid, or Z)" | An accelerometer can't measure yaw; Z is the vertical axis. Yaw needs a gyro or magnetometer | README and sketch comments; pitch and roll computed from gravity |

## License

MIT for the new code — see [LICENSE](LICENSE). `legacy/` is Geeetech's sample code as published on their wiki.
