#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "internal_temp.h"

const char* ssid = "iPhone";
const char* password = "password";
const String serverip = "172.20.10.8:8080";
const String configEndpoint = "http://" +  serverip + "/api/v1/esp32/config";
const String postEndpoint = "http://" +  serverip + "/api/v1/esp32";
const char* mqttBroker = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttTopic = "ynov-lyon-2023/esp32/ed/in";
bool isWifiConnected = false;

WiFiClient wifiClient;
HTTPClient httpClient;
PubSubClient mqttClient(wifiClient);


void connectWifi()
{
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    isWifiConnected = true;
    Serial.println();
    Serial.println("Connected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }
}

void setup()
{
  Serial.begin(115200);
  connectWifi();
  mqttClient.setServer(mqttBroker, mqttPort);
}

void sendTemperatureWithHttp(){
  // Requête HTTP POST avec la valeur de température
      httpClient.begin(postEndpoint.c_str());
      httpClient.addHeader("Content-Type", "application/json");

      // Construction du corps de la requête
      float temperature = readTemp2(false);
      StaticJsonDocument<200> postDoc;
      postDoc["value"] = String(temperature, 2); // Conversion de la température en chaîne avec 2 décimales
      String postBody;
      serializeJson(postDoc, postBody);

      // Envoi de la requête POST
      httpClient.POST(postBody);
}

void sendTemperatureWithMqtt() {
  // Connexion au broker MQTT
      if (!mqttClient.connected())
      {
        Serial.println("Mqtt unconnected");
        mqttClient.connect("esp32-client");
      }

      // Envoi de la température sur le topic MQTT
      float temperature = readTemp2(false);
      Serial.println(temperature);
      String temperatureStr = String(temperature, 2); // Conversion de la température en chaîne avec 2 décimales
      Serial.println("Temperature to send : " + temperatureStr);
      mqttClient.publish(mqttTopic, temperatureStr.c_str());
}

void loop()
{

  if(isWifiConnected) {

    httpClient.begin(configEndpoint.c_str());

    int httpResponseCode = httpClient.GET();
    Serial.println("httpResponseCode :");
    Serial.println(httpResponseCode);
  
    if (httpResponseCode == HTTP_CODE_OK)
    {
      String payload = httpClient.getString();

      // Parsing du JSON reçu
      DynamicJsonDocument doc(200);
      deserializeJson(doc, payload);

      // Extraction des valeurs de configuration
      float tempFreq = doc["tempFreq"];
      int connectionConfig = doc["connectionConfig"];
      int connectionFreq = doc["connectionFreq"];

      Serial.println(tempFreq);


      // Envoi de la température en fonction de la configuration
      if (connectionConfig == 1)
      {
        Serial.println("Start sending data by http");
        sendTemperatureWithHttp();
      }
      else if (connectionConfig == 2)
      {
        Serial.println("Start sending data on topic");
        sendTemperatureWithMqtt();
      }
      // WiFi.disconnect();

      // Attente pendant l'intervalle de tempFreq en millisecondes
      delay(tempFreq * 1000);
    }
    // httpClient.end();

  } else {
    connectWifi();
  }
  
}
