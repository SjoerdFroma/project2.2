#include <WiFi.h>
#include <WiFiUdp.h>
#include <cJSON.h>
#include "secret.h"



#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>



#define NODE_RED_IP "192.168.137.1" //Ip adres van node red dashboard
#define NODE_RED_PORT 49152 // UDP port waarop node red luistert.
#define WIFI_TIMEOUT_MS 20000 // 20 second WiFi connection timeout 20 seconden 
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt


//Vertaald de binnenkomende voltage van de stretch sensor om naar procenten. 
float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Queue om sensor data in te verzamelen.
QueueHandle_t data_queue;

//Aanroepen van de bme280 adafruit.
Adafruit_BME280 bme;

QueueHandle_t audit_queue;

void get_values(void *pvParameters) {

  for (;;) {
    //Haal alle sensor data op en zet het in een variabele.
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100;
    int node = 1;
    float analogValue = analogRead(32);
    float stifness = floatMap(analogValue, 0, 4095, 0, 100);

    //Zet de sensor data in een JSON object en zet het json object om naar een string.
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "node", node);
    cJSON_AddNumberToObject(root, "temp", temperature);
    cJSON_AddNumberToObject(root, "hum", humidity);
    cJSON_AddNumberToObject(root, "pres", pressure);
    cJSON_AddNumberToObject(root, "stretch", stifness);
    char *json_string = cJSON_PrintUnformatted(root);

    //variabele om een queue item in te plaatsen als deze vol is.
    char queue_item_verwijderen;
    //Het aantal vrije plekken in de queue.
    int free_space = uxQueueSpacesAvailable(audit_queue);

    //Check of de queue vol is. Als dat het geval is haal het oudste item uit de queue en zet deze in queue_item_verwijderen.
    //Als dat geslaagd is wordt het nieuwe item naar de queue geschreven. 
    if (free_space == 0) {
      if (xQueueReceive(audit_queue, (void *) &queue_item_verwijderen, 0) == pdTRUE) {
        xQueueSend(audit_queue, (void *)&json_string, 0);
        }
      //Als er nog vrije plek is in de queue schrijf de nieuwe waarde gelijk.
      } else {
          xQueueSend(audit_queue, (void *)&json_string, 0);
          
  }
  //Wacht een seconde voordat nieuwe waardes gelezen worden.
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  }



}

void WiFi_connect(void* parameter){
  for(;;){
    //Een check of de ESP32 nog verbonden is met het internet. En wacht 10 seconden voor een nieuwe check.
    if(WiFi.status() == WL_CONNECTED){
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    //Als er geen verbinding is maak verbinding met het internet.
    WiFi.begin(SSID, PWD);

    unsigned long startAttemptTime = millis();
    //Als het verbinden met internet niet lukt wacht aantal seconden voordat het opnieuw geprobeerd wordt.
    while (WiFi.status() != WL_CONNECTED && 
            millis() - startAttemptTime < WIFI_TIMEOUT_MS){}


      if(WiFi.status() != WL_CONNECTED){
          vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
			      continue;
      }

  }

}


void udp_json_task(void *pvParameters)
{
    //Start de module udp met het node red udp port.
    WiFiUDP udp;
    udp.begin(NODE_RED_PORT);
    //Variabele waar de json string uit de queue in gezet kan worden.
    char *json_string;

    while (1) {
      //Check of er nog internet verbinding is als dat het geval is wordt er een item uit de queue gehaald en via de udp verstuurd naar node red.
      if(WiFi.status() == WL_CONNECTED){

        int free_space = uxQueueMessagesWaiting(audit_queue);

        if (free_space > 0) {

          xQueueReceive(audit_queue, (void *) &json_string, 10);

          udp.beginPacket(NODE_RED_IP, NODE_RED_PORT);
          udp.write((const uint8_t *)json_string, strlen(json_string));
          udp.endPacket();
          vTaskDelay(500 / portTICK_PERIOD_MS);
        }

    }


}
  vTaskDelete(NULL);
}

void setup() {
  //Bme starten
  bme.begin(0x76);
  int app_cpu = xPortGetCoreID();
  BaseType_t rc;
  audit_queue = xQueueCreate(200, (4 *sizeof(char)));
  delay(2000); // Allow USB to connect
  //Task voor wifi verbinden/controle
  xTaskCreatePinnedToCore(WiFi_connect, "WiFi_connect_task", 5000, NULL, 15, NULL, app_cpu);
  //Task voor het verzamelen van sensor data.
  xTaskCreatePinnedToCore(get_values, "get_values", 8192, NULL, 5, NULL, app_cpu);
  //Task voor het verzenden van de sensor data.
  xTaskCreatePinnedToCore(udp_json_task, "udp_json_task", 8192, NULL, 5, NULL, app_cpu);




}

void loop (){
  vTaskDelete(nullptr);
}
