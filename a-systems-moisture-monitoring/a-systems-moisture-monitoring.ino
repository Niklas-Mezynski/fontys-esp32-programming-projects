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

// Comment in/out for toggling debug mode (Serial prints and so on)
// #define DEBUG

// WiFi Connection
const char *ssid = "MagentaWLAN-Mezynski";
const char *password = "MANS4mezy";

// HTTP definitions
const char *url = "https://webserver-a-systems-individual-production.up.railway.app/trpc/sensors.addSensorData";
const char *apiAuthorization = "E3o8zuQp3W2izyvmBK0DqGhFecWtEqEa";

// Sensor definitions (General)
const int MILLIS_BETWEEN_READINGS = 5000;       // 5 Seconds
const int MILLIS_BETWEEN_DATA_SENDING = 300000; // 5 Minutes
// const int MILLIS_BETWEEN_READINGS = 5000;      // 5 Seconds - For TESTING!
// const int MILLIS_BETWEEN_DATA_SENDING = 60000; // 1 Minute - For TESTING!
const int NUM_READINGS = MILLIS_BETWEEN_DATA_SENDING / MILLIS_BETWEEN_READINGS;

// Sensor definitions (Main Sensor)
#define AOUT_PIN 36 // Analog input pin that the sensor output is attached to(white wire)

bool is1V1Output = false; // set true if 1.1V output sensor is used for 3V set to false

float refVoltage = 3.3; // set reference voltage - by default Arduino supply voltage (UNO - 5V)

int minADC = 0;    // replace with min ADC value read in air
int maxADC = 3300; // replace with max ADC value read fully submerged in water

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

#ifdef DEBUG
  Serial.printf("Raw moisture value: %d (Avg: %.3f) | Moisture value = %d (Avg: %.3f)\n", rawValue, averageRawMain, mappedValue, averagePercMain);
#endif
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
