#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESPTools.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
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
        String topics[4] = {
            ESPTools.config["mqtt_topic_relay_1"],
            ESPTools.config["mqtt_topic_relay_2"],
            ESPTools.config["mqtt_topic_relay_3"],
            ESPTools.config["mqtt_topic_relay_4"]
        };

        for (String topic : topics) {
            if (!topic.equals("")) {
                client.subscribe(topic.c_str());
            }
        }
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" trying again");
        delay(100);
    }
}

void switchRelay(int pin, bool state)
{
    if (state)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        digitalWrite(pin, LOW);
    }
}

void onMessage(char *topic, byte *payload, unsigned int length)
{
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, (char *)payload);

    if (ESPTools.config["mqtt_topic_relay_1"].equals(topic))
    {
        switchRelay(RELAY_1, doc["state"]);
    }
    if (ESPTools.config["mqtt_topic_relay_2"].equals(topic))
    {
        switchRelay(RELAY_2, doc["state"]);
    }
    if (ESPTools.config["mqtt_topic_relay_3"].equals(topic))
    {
        switchRelay(RELAY_3, doc["state"]);
    }
    if (ESPTools.config["mqtt_topic_relay_4"].equals(topic))
    {
        switchRelay(RELAY_4, !doc["state"]);
    }
}

void setup()
{
    // WiFi, WebServer and OTA setup
    Serial.begin(115200);
    delay(1000);

    // Setup Pins
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);
    digitalWrite(RELAY_4, HIGH);

    ESPTools.begin(&server);
    ESPTools.setupHTTPUpdates();
    ESPTools.wifiAutoConnect();
    ESPTools.addConfigString("mqtt_server");
    ESPTools.addConfigString("mqtt_topic_relay_1");
    ESPTools.addConfigString("mqtt_topic_relay_2");
    ESPTools.addConfigString("mqtt_topic_relay_3");
    ESPTools.addConfigString("mqtt_topic_relay_4");

    server.on("/restart", [&]()
              {
        server.send(200, "text/plain", "Ok");
        delay(500);
        ESP.restart(); });

    server.begin();

    // Setup MQTT
    reconnect();
    client.setCallback(onMessage);
}

void loop()
{
    server.handleClient();
    if (!client.connected() && ESPTools.config["mqtt_server"] != "" && WiFi.status() == WL_CONNECTED)
    {
        reconnect();
    }
    client.loop();
}