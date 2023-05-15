#include <ArduinoJson.h>

#include <HTTPClient.h>

#include "WiFi.h"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Wire.h>

// Comment in/out for toggling debug mode (Serial prints and so on)
#define DEBUG

// WiFi Connection
const char *ssid = "MagentaWLAN-Mezynski";
const char *password = "MANS4mezy";

// HTTP definitions
// const char *url = "https://webserver-a-systems-individual-production.up.railway.app/trpc/sensors.addSensorData";
const char *url = "http://192.168.2.52:3002/trpc/sensors.addSensorData"; // For TESTING!
const char *apiAuthorization = "E3o8zuQp3W2izyvmBK0DqGhFecWtEqEa";

// Sensor definitions (General)
// const int MILLIS_BETWEEN_READINGS = 3000;       // 3 Seconds
// const int MILLIS_BETWEEN_DATA_SENDING = 300000; // 5 Minutes
const int MILLIS_BETWEEN_READINGS = 1000;      // 1 Seconds - For TESTING!
const int MILLIS_BETWEEN_DATA_SENDING = 60000; // 1 Minute - For TESTING!
const int NUM_READINGS = MILLIS_BETWEEN_DATA_SENDING / MILLIS_BETWEEN_READINGS;

// Class or struct "Sensor" that contains the sensor data and the sensor type
class Sensor
{
private:
  int idx;

public:
  uint8_t pin;
  String sensorName;
  int readings[NUM_READINGS] = {0};
  int readingsPerc[NUM_READINGS] = {0};
  int total;
  int totalPerc;
  int minADC;
  int maxADC;
  Sensor(uint8_t pin, String sensorName, int minADC, int maxADC)
  {
    this->pin = pin;
    this->sensorName = sensorName;
    this->minADC = minADC;
    this->maxADC = maxADC;
    this->total = 0;
    this->totalPerc = 0;
    this->idx = 0;
  }

  // Function that reads the sensor value
  int readSensor()
  {
    int rawValue = analogRead(this->pin);
    this->total = this->total - this->readings[this->idx] + rawValue;
    this->readings[this->idx] = rawValue;

    int percValue = map(rawValue, this->minADC, this->maxADC, 0, 100);

    this->totalPerc = this->totalPerc - this->readingsPerc[this->idx] + percValue;
    this->readingsPerc[this->idx] = percValue;

    this->idx = (this->idx + 1) % NUM_READINGS;

    return rawValue;
  }

  // Function that calculates the rolling average
  float getRollingAverage()
  {

    return this->total / (float)NUM_READINGS;
  }

  // Function that calculates the percentage
  float getPercentageAverage()
  {
    return this->totalPerc / (float)NUM_READINGS;
  }

  void postSensorData()
  {
    float averageRaw = this->getRollingAverage();
    float averagePerc = this->getPercentageAverage();

    // Create a JSON document
    StaticJsonDocument<200> doc;
    doc["rawValue"] = static_cast<int>(averageRaw);
    doc["humidity"] = averagePerc;
    doc["sensorType"] = this->sensorName;

    // Serialize the JSON document to a string
    String json;
    serializeJson(doc, json);

    // Send an HTTP POST request to the REST API
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", apiAuthorization);
    int httpCode = http.POST(json);
#ifdef DEBUG
    if (httpCode > 0)
    {
      Serial.printf("[HTTP] POST request sent, status code: %d\n", httpCode);
      String response = http.getString();
      Serial.println(response);
    }
    else
    {
      Serial.printf("[HTTP] POST request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
#endif
    http.end();
  }

  void printSensorData()
  {
#ifdef DEBUG
    float avg = this->getRollingAverage();
    float averagePerc = this->getPercentageAverage();

    int lastReading = this->readings[(this->idx + NUM_READINGS - 1) % NUM_READINGS];
    int percValue = map(lastReading, this->minADC, this->maxADC, 0, 100);

    Serial.printf("%s Sensor -- Moisture = %d\% [Avg: %.2f\%] | (Raw: %d [Avg: %.2f])\n", this->sensorName, percValue, averagePerc, lastReading, avg);
#endif
  }
};

// Sensor definitions (Main Sensor)

/**
 * -- ESP32 connections --
 * Brown (VCC) -> +3.3V (3V3)
 * White (Output) -> GPIO36 (SP)
 * Green (GND) -> GND (GND)
 */

// For ADC -> use pins (GPIO 0, 2, 4, 12-15, 25-27, 32-39)

std::vector<Sensor> sensors;

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif

  // Initialize the sensors
  // Sensor s1 = Sensor(36, "main", 0, 3500);
  Sensor s2 = Sensor(36, "second", 4095, 2045);

  sensors.push_back(s2);

  connectWiFi();

  // Initialize the sensor readings to 0

#ifdef DEBUG
  Serial.println("Setup done!");
#endif
}

void loop()
{
  for (auto &sensor : sensors)
  {
    sensor.readSensor();
#ifdef DEBUG
    sensor.printSensorData();
#endif
  }

  sendDataToServer();

  delay(MILLIS_BETWEEN_READINGS);
}

unsigned long lastTime = millis(); // keep track of the last time the action was performed
void sendDataToServer()
{
  // get the current time
  unsigned long currentTime = millis();

  // check if 5 minutes have passed since the last action
  if (currentTime - lastTime >= MILLIS_BETWEEN_DATA_SENDING)
  {
    for (auto &sensor : sensors)
    {
      sensor.postSensorData();
    }
    lastTime = currentTime;
  }
}

void connectWiFi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    switch (WiFi.status())
    {
    case WL_CONNECT_FAILED:
#ifdef DEBUG
      Serial.println("WiFi connection failed!");
#endif
      break;
    default:
      delay(500);
#ifdef DEBUG
      Serial.println("Connecting to WiFi..");
#endif
    }
  }

#ifdef DEBUG
  Serial.println("Connected to the WiFi network");
#endif
}

// -- SECOND SENSOR STUFF (deprecated) --

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

#define MIN_2ND 191 //-66
#define MAX_2ND 480 //

int rawValue2nd, mappedValue2nd;

int readings2nd[NUM_READINGS];
int readingsPerc2nd[NUM_READINGS];
int idx2nd = 0;
int total2nd = 0;
int totalPerc2nd = 0;

const char *sensorName2nd = "second";
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
