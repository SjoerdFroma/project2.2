#include <WiFi.h>
#include <WiFiUdp.h>
#include <cJSON.h>
#include "secret.h"


#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>


#define NODE_RED_IP "192.168.137.126"
#define NODE_RED_PORT 49152

Adafruit_BME280 bme;

// env variable
float temperature;
float humidity;
float pressure;
float stifness;
float analogValue;


float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



void WiFi_connect(void* parameter){
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      printf(".");
  }
  printf(" CONNECTED\r\n");
  vTaskDelete(nullptr);

}


void udp_json_task(void *pvParameters)
{
    WiFiUDP udp;
    udp.begin(NODE_RED_PORT);


    while (1) {


      temperature = bme.readTemperature();
      humidity = bme.readHumidity();
      pressure = bme.readPressure() / 100;
      int node = 1;
      analogValue = analogRead(32);

      stifness = floatMap(analogValue, 0, 4095, 0, 100);
      cJSON *root = cJSON_CreateObject();
      cJSON_AddNumberToObject(root, "node", node);
      cJSON_AddNumberToObject(root, "temp", temperature);
      cJSON_AddNumberToObject(root, "hum", humidity);
      cJSON_AddNumberToObject(root, "pres", pressure);
      cJSON_AddNumberToObject(root, "stretch", stifness);
      char *json_string = cJSON_PrintUnformatted(root);

      udp.beginPacket(NODE_RED_IP, NODE_RED_PORT);
      udp.write((const uint8_t *)json_string, strlen(json_string));
      udp.endPacket();
      vTaskDelay(1000 / portTICK_PERIOD_MS);

      cJSON_Delete(root);
    }

    vTaskDelete(NULL);
}

void setup() {
  bme.begin(0x76);
  int app_cpu = xPortGetCoreID();
  BaseType_t rc;
  Serial.begin(115200);
  delay(2000); // Allow USB to connect
  xTaskCreatePinnedToCore(WiFi_connect, "WiFi_connect_task", 4096, NULL, 15, NULL, app_cpu);

  xTaskCreatePinnedToCore(udp_json_task, "udp_json_task", 4096, NULL, 5, NULL, app_cpu);

}

void loop (){
  vTaskDelete(nullptr);
}
