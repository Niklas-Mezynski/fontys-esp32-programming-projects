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

// General
const char *ssid = "MagentaWLAN-Mezynski";
const char *password = "MANS4mezy";

// Test Socker Server
const char *clientIP = "192.168.2.52";
int sock = -1;

// Test HTTP
const char *url = "http://192.168.2.52:3000/trpc/sensors.getSensorData";

// Sensor stuff
#define AOUT_PIN 36 // Analog input pin that the sensor output is attached to(white wire)

bool is1V1Output = false; // set true if 1.1V output sensor is used for 3V set to false

float refVoltage = 3.3; // set reference voltage - by default Arduino supply voltage (UNO - 5V)

int minADC = 0;    // replace with min ADC value read in air
int maxADC = 3300; // replace with max ADC value read fully submerged in water

int rawValue, mappedValue;

const int NUM_READINGS = 20;

int readings[NUM_READINGS];
int readingsPerc[NUM_READINGS];
int idx = 0;
int total = 0;
int totalPerc = 0;

void setup()
{

  Serial.begin(115200);

  // connectWiFi();

  // initSocket();

  Serial.println("Setup done!");
}

void loop()
{
  // sendMessageToSocketTest();
  // performHTTP_GET_Request();
  readSensorData();
  delay(500);
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
  float average = total / (float)NUM_READINGS;
  float averagePerc = totalPerc / (float)NUM_READINGS;

  Serial.printf("Raw moisture value: %d (Avg: %.3f) | Moisture value = %d (Avg: %.3f)\n", rawValue, average, mappedValue, averagePerc);
}

void performHTTP_GET_Request()
{
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    Serial.printf("[HTTP] GET request sent, status code: %d\n", httpCode);
    String response = http.getString();
    Serial.println(response);

    // Parse the JSON response
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.printf("[JSON] Failed to parse JSON response, error: %s\n", error.c_str());
    }
    else
    {
      // Get the sensor value from the JSON response
      JsonArray data = doc["result"]["data"];
      for (JsonVariant v : data)
      {
        int id = v["id"];
        const char *name = v["name"];
        Serial.printf("[JSON] id: %d, name: %s\n", id, name);
      }
    }
  }
  else
  {
    Serial.printf("[HTTP] GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void sendMessageToSocketTest()
{
  char *name = "Argus Suave";
  int rand_num = rand() % 100;

  char message[100];
  sprintf(message, "%s %d", name, rand_num);
  if (send(sock, message, strlen(message), 0) < 0)
  {
    Serial.println("Error: Failed to send message");
    return;
  }

  Serial.println("Message sent! (" + String(message) + ")");
}

void connectWiFi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    switch (WiFi.status())
    {
    case WL_CONNECT_FAILED:
      Serial.println("WiFi connection failed!");
      break;
    default:
      delay(500);
      Serial.println("Connecting to WiFi..");
    }
  }

  Serial.println("Connected to the WiFi network");
}

void initSocket()
{
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    Serial.println("Error: Failed to create socket");
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(1234);

  if (inet_pton(AF_INET, clientIP, &server_addr.sin_addr) <= 0)
  {
    Serial.println("Error: Invalid address");
    return;
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    Serial.println("Error: Failed to connect to server");
    return;
  }
}