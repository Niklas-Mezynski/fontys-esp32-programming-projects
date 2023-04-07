#include "WiFi.h"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const char* ssid = "MagentaWLAN-Mezynski";
const char* password =  "MANS4mezy";
int sock = -1;

void setup() {
 
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    switch(WiFi.status()) {
      case WL_CONNECT_FAILED:
        Serial.println("WiFi connection failed!");
        break;
      default:
        delay(500);
        Serial.println("Connecting to WiFi..");
      }
  }
 
  Serial.println("Connected to the WiFi network");
 
  sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        Serial.println("Error: Failed to create socket");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    if (inet_pton(AF_INET, "192.168.2.73", &server_addr.sin_addr) <= 0) {
        Serial.println("Error: Invalid address");
        return;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Serial.println("Error: Failed to connect to server");
        return;
    }

}
 
void loop() {
    char* name = "Argus Suave";
    int rand_num = rand() % 100;

    char message[100];
    sprintf(message, "%s %d", name, rand_num);
    if (send(sock, message, strlen(message), 0) < 0) {
        Serial.println("Error: Failed to send message");
        return;
    }

    Serial.println("Message sent!");

    delay(2000);
    // close(sock);

}