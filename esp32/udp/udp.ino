#include <WiFi.h>
#include <WiFiUdp.h>
#include <cJSON.h>
#include "secret.h"


#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>


#define NODE_RED_IP "192.168.137.1"
#define NODE_RED_PORT 49152

Adafruit_BME280 bme;

// env variable
float temperature;
float humidity;
float pressure;


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

      cJSON *root = cJSON_CreateObject();
      
      cJSON *temp = cJSON_CreateObject();
      cJSON_AddStringToObject(temp, "Type", "temperature");
      cJSON_AddStringToObject(temp, "Symb", "°C");
      cJSON_AddNumberToObject(temp, "Type", temperature);

      cJSON *hum = cJSON_CreateObject();
      cJSON_AddStringToObject(hum, "Type", "humidity");
      cJSON_AddStringToObject(hum, "Symb", "%");
      cJSON_AddNumberToObject(hum, "Type", humidity);

      cJSON *pres = cJSON_CreateObject();
      cJSON_AddStringToObject(pres, "Type", "pressure");
      cJSON_AddStringToObject(pres, "Symb", "mBar");
      cJSON_AddNumberToObject(pres, "Type", pressure);

      cJSON_AddItemToObject(root, "temp", temp);
      cJSON_AddItemToObject(root, "hum", hum);
      cJSON_AddItemToObject(root, "pres", pres);

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
  delay(2000); // Allow USB to connect
  xTaskCreatePinnedToCore(WiFi_connect, "WiFi_connect_task", 4096, NULL, 15, NULL, app_cpu);

  xTaskCreatePinnedToCore(udp_json_task, "udp_json_task", 4096, NULL, 5, NULL, app_cpu);

}

void loop (){
  vTaskDelete(nullptr);
}