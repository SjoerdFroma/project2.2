#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include "secret.h"
 
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>


#define PIN 4
 
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
 
void WiFi_connect() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.print("Connected. Het ip adres van de esp is: ");
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
 
     // delay the task
     vTaskDelay(60000 / portTICK_PERIOD_MS);
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

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }

  String body = server.arg("plain");
  Serial.println(body);
  deserializeJson(jsonDocument, body);
  
  // Get RGB components
  int red = jsonDocument["red"];
  int green = jsonDocument["green"];
  int blue = jsonDocument["blue"];

  Serial.print("Red: ");
  Serial.print(red);
  

  pixels.fill(pixels.Color(red, green, blue));
  delay(30);
  pixels.show();

  // Respond to the client
  server.send(200, "application/json", "{}");
}
 
 
// setup API resources
void setup_routing() {
  server.on("/temperature", getTemperature);
  server.on("/pressure", getPressure);
  server.on("/humidity", getHumidity);
  server.on("/env", getEnv);
  server.on("/led", HTTP_POST, handlePost);
 
  // start server
  server.begin();
}


void setup_task() {
  xTaskCreate(
    read_sensor_data,    
    "Read sensor data",   // Name of the task (for debugging)
    1000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
}
 
void setup() {
   Serial.begin(9600);
 
   // Sensor setup
  if (!bme.begin(0x76)) {
    Serial.println("Problem connecting to BME280");
  }
  WiFi_connect();
  setup_task();
  setup_routing();  

  // Initialize Neopixel
  pixels.begin();
}
 
void loop() {
  server.handleClient();
 
}