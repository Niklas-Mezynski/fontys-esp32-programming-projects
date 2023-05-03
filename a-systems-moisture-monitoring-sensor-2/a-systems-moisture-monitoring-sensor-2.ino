#include <Wire.h>

#define DEBUG

// Sensor definitions (General)
const int MILLIS_BETWEEN_READINGS = 1000;      // 5 Seconds
const int MILLIS_BETWEEN_DATA_SENDING = 20000; // 5 Minutes
// const int MILLIS_BETWEEN_READINGS = 5000;      // 5 Seconds - For TESTING!
// const int MILLIS_BETWEEN_DATA_SENDING = 60000; // 1 Minute - For TESTING!
const int NUM_READINGS = MILLIS_BETWEEN_DATA_SENDING / MILLIS_BETWEEN_READINGS;

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  resetSecondSensor();
}

void loop()
{
  readSecondSensorData();
  delay(MILLIS_BETWEEN_READINGS);
}

// -- SECOND SENSOR STUFF --

/**
 * -- ESP32 connections --
 * Left -> Right
 * Red: VCC -> +5V (V5)
 * White: -> S CL (G22)
 * Green: -> S DA (G21)
 * Black: GND -> GND (GND)
 */

#define ADDR_2ND 0x20 // Input addr for I2C channel
#define RESET_REG 6
#define CAPACITANCE_REG 0

#define MIN_2ND 191
#define MAX_2ND 480

int rawValue2nd, mappedValue2nd;

int readings2nd[NUM_READINGS];
int readingsPerc2nd[NUM_READINGS];
int idx2nd = 0;
int total2nd = 0;
int totalPerc2nd = 0;

const char *sensorNameMain = "main";
float averageRaw2nd = 0;
float averagePerc2nd = 0;

void resetSecondSensor()
{
  writeI2CRegister8bit(ADDR_2ND, RESET_REG); // reset
}

void readSecondSensorData()
{
  rawValue2nd = readI2CRegister16bit(ADDR_2ND, CAPACITANCE_REG); // read the value from sensor

  mappedValue2nd = map(rawValue2nd, MIN_2ND, MAX_2ND, 0, 100);

  // Add the reading to the rolling average
  total2nd = total2nd - readings2nd[idx2nd] + rawValue2nd;
  readings2nd[idx2nd] = rawValue2nd;

  totalPerc2nd = totalPerc2nd - readingsPerc2nd[idx2nd] + mappedValue2nd;
  readingsPerc2nd[idx2nd] = mappedValue2nd;

  idx2nd = (idx2nd + 1) % NUM_READINGS;

  // Calculate the rolling average
  averageRaw2nd = total2nd / (float)NUM_READINGS;
  averagePerc2nd = totalPerc2nd / (float)NUM_READINGS;

#ifdef DEBUG
  Serial.printf("2nd Sensor -- Raw moisture value: %d (Avg: %.3f) | Moisture value = %d (Avg: %.3f)\n", rawValue2nd, averageRaw2nd, mappedValue2nd, averagePerc2nd);
#endif
}

void writeI2CRegister8bit(int addr, int value)
{
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

unsigned int readI2CRegister16bit(int addr, int reg)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  delay(20);
  Wire.requestFrom(addr, 2);
  unsigned int t = Wire.read() << 8;
  t = t | Wire.read();
  return t;
}