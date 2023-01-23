#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "secret.h"
#include <Wire.h>
 
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

//#define PIN 4
 
// Web server running on port 80
WebServer server(80);
 
// Sensor
Adafruit_BME280 bme;


 
// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];
 
// env variable
float temperature;
float humidity;
float pressure;
 
void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

}
 
void create_json(char *tag, float value, char *unit) { 
  jsonDocument.clear(); 
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
  Serial.println("Buffer:");
  Serial.println(buffer);  
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void read_sensor_data(void * parameter) {
   for (;;) {
     temperature = bme.readTemperature();
     humidity = bme.readHumidity();
     pressure = bme.readPressure() / 100;
     Serial.println("Read sensor data");
     Serial.println(temperature);
 
     // delay the task
     vTaskDelay(6000 / portTICK_PERIOD_MS);

   }
}
 
void getTemperature() {
  Serial.println("Get temperature");
  create_json("temperature", temperature, "°C");
  server.send(200, "application/json", buffer);
}
 
void getHumidity() {
  Serial.println("Get humidity");
  create_json("humidity", humidity, "%");
  server.send(200, "application/json", buffer);
}
 
void getPressure() {
  Serial.println("Get pressure");
  create_json("pressure", pressure, "mBar");
  server.send(200, "application/json", buffer);
}
 
void getEnv() {
  Serial.println("Get env");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  add_json_object("humidity", humidity, "%");
  add_json_object("pressure", pressure, "mBar");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}


// setup API resources
void setup_routing() {
  server.on("/temperature", getTemperature);
  server.on("/pressure", getPressure);
  server.on("/humidity", getHumidity);
  server.on("/env", getEnv);

 
  // start server
  server.begin();
}


void setup_task() {
  xTaskCreate(
    read_sensor_data,    
    "Read sensor data",   // Name of the task (for debugging)
    4096,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
}
 
void setup() {
   bme.begin(0x76);
   Serial.begin(9600);
 
   // Sensor setup
  if (!bme.begin(0x76)) {
    Serial.println("Problem connecting to BME280");

  }
  connectToWiFi();
  setup_task();
  setup_routing();  


}
 
void loop() {
  server.handleClient();

 
}