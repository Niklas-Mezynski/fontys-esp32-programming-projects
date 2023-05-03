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
// #define DEBUG

// WiFi Connection
const char *ssid = "MagentaWLAN-Mezynski";
const char *password = "MANS4mezy";

// HTTP definitions
const char *url = "https://webserver-a-systems-individual-production.up.railway.app/trpc/sensors.addSensorData";
// const char *url = "http://192.168.2.52:3002/trpc/sensors.addSensorData"; // For TESTING!
const char *apiAuthorization = "E3o8zuQp3W2izyvmBK0DqGhFecWtEqEa";

// Sensor definitions (General)
const int MILLIS_BETWEEN_READINGS = 3000;       // 3 Seconds
const int MILLIS_BETWEEN_DATA_SENDING = 300000; // 5 Minutes
// const int MILLIS_BETWEEN_READINGS = 1000;      // 1 Seconds - For TESTING!
// const int MILLIS_BETWEEN_DATA_SENDING = 60000; // 1 Minute - For TESTING!
const int NUM_READINGS = MILLIS_BETWEEN_DATA_SENDING / MILLIS_BETWEEN_READINGS;

// Sensor definitions (Main Sensor)

/**
 * -- ESP32 connections --
 * Brown (VCC) -> +3.3V (3V3)
 * White (Output) -> GPIO36 (SP)
 * Green (GND) -> GND (GND)
 */

#define AOUT_PIN 36 // Analog input pin that the sensor output is attached to(white wire)

bool is1V1Output = false; // set true if 1.1V output sensor is used for 3V set to false

float refVoltage = 3.3; // set reference voltage - by default Arduino supply voltage (UNO - 5V)

int minADC = 0;    // replace with min ADC value read in air
int maxADC = 3500; // replace with max ADC value read fully submerged in water

int rawValue, mappedValue;

int readings[NUM_READINGS];
int readingsPerc[NUM_READINGS];
int idx = 0;
int total = 0;
int totalPerc = 0;

const char *sensorNameMain = "main";
float averageRawMain = 0;
float averagePercMain = 0;

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif
  connectWiFi();

#ifdef DEBUG
  Serial.println("Setup done!");
#endif
}

void loop()
{
  readSensorData();
  // readSecondSensorData();
#ifdef DEBUG
  printSensorData();
#endif

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
    performHTTP_POST_Request();
    lastTime = currentTime;
  }
}

void readSensorData()
{
  rawValue = analogRead(AOUT_PIN); // read the analog value from sensor

  mappedValue = map(rawValue, minADC, maxADC, 0, 100);

  // Add the reading to the rolling average
  total = total - readings[idx] + rawValue;
  readings[idx] = rawValue;

  totalPerc = totalPerc - readingsPerc[idx] + mappedValue;
  readingsPerc[idx] = mappedValue;

  idx = (idx + 1) % NUM_READINGS;

  // Calculate the rolling average
  averageRawMain = total / (float)NUM_READINGS;
  averagePercMain = totalPerc / (float)NUM_READINGS;
}

void performHTTP_POST_Request()
{
  // Create a JSON document
  StaticJsonDocument<200> doc;
  doc["rawValue"] = static_cast<int>(averageRawMain);
  doc["humidity"] = averagePercMain;
  doc["sensorType"] = sensorNameMain;

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

void printSensorData()
{
  Serial.printf("Main Sensor -- Moisture = %d\% [Avg: %.3f\%] | (Raw: %d [Avg: %.3f])", mappedValue, averagePercMain, rawValue, averageRawMain);
  Serial.printf(" - ");
  Serial.printf("2nd Sensor -- Moisture = %d\% [Avg: %.3f\%] | (Raw: %d [Avg: %.3f])\n", mappedValue2nd, averagePerc2nd, rawValue2nd, averageRaw2nd);
}