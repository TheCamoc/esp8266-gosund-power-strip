#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESPTools.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

const int RELAY_1 = 14;
const int RELAY_2 = 12;
const int RELAY_3 = 13;
const int RELAY_4 = 5;

void reconnect()
{
    Serial.print("Attempting MQTT connection... ");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    client.setServer(ESPTools.config["mqtt_server"].c_str(), 1883);
    if (client.connect(clientId.c_str()))
    {
        Serial.println("connected");
        client.subscribe(ESPTools.config["mqtt_topic_relay_1"].c_str());
        client.subscribe(ESPTools.config["mqtt_topic_relay_2"].c_str());
        client.subscribe(ESPTools.config["mqtt_topic_relay_3"].c_str());
        client.subscribe(ESPTools.config["mqtt_topic_relay_4"].c_str());
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" trying again");
        delay(100);
    }
}

void switchRelay(int pin, bool state) {
    if (state) {
        digitalWrite(pin, HIGH);
    } else {
        digitalWrite(pin, LOW);
    }
}

void onMessage(char *topic, byte *payload, unsigned int length)
{
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, (char *)payload);

    if (std::strcmp(ESPTools.config["mqtt_topic_relay_1"].c_str(), topic) == 0) {
        switchRelay(RELAY_1, doc["state"]);
    } 
    if (std::strcmp(ESPTools.config["mqtt_topic_relay_2"].c_str(), topic) == 0) {
        switchRelay(RELAY_2, doc["state"]);
    } 
    if (std::strcmp(ESPTools.config["mqtt_topic_relay_3"].c_str(), topic) == 0) {
        switchRelay(RELAY_3, doc["state"]);
    }
    if (std::strcmp(ESPTools.config["mqtt_topic_relay_4"].c_str(), topic) == 0) {
        switchRelay(RELAY_4, doc["state"]);
    }
}

void resetWifi()
{
    server.send(200, "text/plain", "done");
    delay(100);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(5000);
    ESP.restart();
}

void setup()
{
    // WiFi, WebServer and OTA setup
    Serial.begin(115200);
    Serial.println();
    delay(1000);

    // Setup Pins
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);

    ESPTools.begin(&server);
    ESPTools.addConfigString("hostname");
    ESPTools.addConfigString("mqtt_server");
    ESPTools.addConfigString("mqtt_topic_relay_1");
    ESPTools.addConfigString("mqtt_topic_relay_2");
    ESPTools.addConfigString("mqtt_topic_relay_3");
    ESPTools.addConfigString("mqtt_topic_relay_4");
    ESPTools.wifiAutoConnect();

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/reset_wifi", resetWifi);
    server.on("/restart", [&]() {
        server.send(200, "text/plain", "Ok");
        delay(500);
        ESP.restart();
    });

    // OTA Stuff
    httpUpdater.setup(&server);
    server.begin();

    // Setup MQTT
    reconnect();
    client.setCallback(onMessage);
}

void loop()
{
    server.handleClient();
    if (!client.connected() && ESPTools.config["mqtt_server"] != "")
    {
        reconnect();
    }
    client.loop();
}