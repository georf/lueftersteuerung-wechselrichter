#include "main.h"

// Aktuelle Zeit immer nur einmal pro Loop lesen
unsigned long now;

// Debug-Ausgaben nur alle 30 Sekunden
unsigned long lastTemperaturCheck = 0;
const unsigned long TEMPERATUR_CHECK_INTERVAL = 3 * 1000; // 3 Sekunden

const float R0 = 12000.0; // Widerstand bei 25°C
const float T0 = 298.15;  // 25°C in Kelvin
const float B = 3950.0;   // B-Wert
uint8 fanPercent = 0;
boolean fanPercentManual = false;
float lastTemperature = 0.0;

float readTemperature()
{
  int adc = analogRead(A0);

  float rNtc =
      12000.0 * adc / (1023.0 - adc);

  float temperatureK = 1.0 / (1.0 / T0 + log(rNtc / R0) / B);

  float temperature = temperatureK - 273.15;
  float correctedTemperature = temperature * 1.2381 - 5.2857;
  return correctedTemperature;
}

// ----------------------------------------------------------
// WLAN & MQTT
// ----------------------------------------------------------
#define WLAN_RECONNECT_TIME 10000           // 10 Sekunden
#define MQTT_RECONNECT_TIME 5000            // 5 Sekunden
#define MQTT_STATUS_INTERVAL 10 * 60 * 1000 // 10 Minuten
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastWifiReconnect = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastMqttStatusUpdate = 0;

void mqttSendAdebarLueftersteuerungFanPercent(boolean full);

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  if (!strcmp(topic, "adebar/lueftersteuerung/system/set"))
  {
    if (!strncmp((char *)payload, "RESTART", length))
    {
      ESP.restart();
    }
    return;
  }

  if (!strcmp(topic, "adebar/lueftersteuerung/fan_percent/set"))
  {

    if (!strncmp((char *)payload, "AUTO", length))
    {
      fanPercentManual = false;
      return;
    }
    uint8_t newFanPercent = atoi((char *)payload);
    if (newFanPercent > 100)
      newFanPercent = 100;
    fanPercent = newFanPercent;
    fanPercentManual = true;
    analogWrite(D6, fanPercent * 255 / 100);
    mqttSendAdebarLueftersteuerungFanPercent(false);

    return;
  }
}

bool connectWifiNonBlocking()
{
  if (WiFi.status() == WL_CONNECTED)
    return true;

  if (now - lastWifiReconnect < WLAN_RECONNECT_TIME)
    return false;

  lastWifiReconnect = now;
  WiFi.begin(wifiSsid, wifiPassword);
  return false;
}

// MQTT senden
bool mqttPublish(const char *topic, const char *message)
{
  if (mqttClient.connected())
  {
    mqttClient.publish(topic, message);
    return true;
  }
  else
  {
    return false;
  }
}

// MQTT Debug senden
bool mqttDebug(const char *message)
{
  return mqttPublish("adebar/lueftersteuerung/system/debug", message);
}

void mqttSendAdebarLueftersteuerungIpAddress(boolean full)
{
  if (full)
  {
    // see https://www.home-assistant.io/integrations/sensor.mqtt/
    JsonDocument discoveryConfig;
    discoveryConfig["name"] = "Lueftersteuerung IP-Adresse";
    discoveryConfig["stat_t"] = "adebar/lueftersteuerung/ip_address/state";
    discoveryConfig["uniq_id"] = "adebar_lueftersteuerung_ip_address";

    JsonObject device = discoveryConfig["dev"].to<JsonObject>();
    device["identifiers"][0] = "adebar_lueftersteuerung";
    device["name"] = "Lueftersteuerung";

    char buffer[256];
    serializeJson(discoveryConfig, buffer);
    mqttPublish("homeassistant/sensor/adebar_lueftersteuerung_ip_address/config", buffer);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    String ip = WiFi.localIP().toString();
    char ip_char[ip.length() + 1];
    ip.toCharArray(ip_char, ip.length() + 1);

    mqttPublish("adebar/lueftersteuerung/ip_address/state", ip_char);
  }
}

void mqttSendAdebarLueftersteuerungTemperature(boolean full)
{
  if (full)
  {
    // see https://www.home-assistant.io/integrations/sensor.mqtt/
    JsonDocument discoveryConfig;
    discoveryConfig["name"] = "Lueftersteuerung Temperatur";
    discoveryConfig["dev_cla"] = "temperature";
    discoveryConfig["stat_t"] = "adebar/lueftersteuerung/temperature/state";
    discoveryConfig["uniq_id"] = "adebar_lueftersteuerung_temperature";
    discoveryConfig["state_class"] = "measurement";
    discoveryConfig["unit_of_measurement"] = "°C";

    JsonObject device = discoveryConfig["dev"].to<JsonObject>();
    device["identifiers"][0] = "adebar_lueftersteuerung";
    device["name"] = "Lueftersteuerung";

    char buffer[512];
    serializeJson(discoveryConfig, buffer);
    mqttPublish("homeassistant/sensor/adebar_lueftersteuerung_temperatur/config", buffer);
  }

  char temperatureString[16];
  snprintf(
      temperatureString,
      sizeof(temperatureString),
      "%.2f",
      lastTemperature);

  mqttPublish("adebar/lueftersteuerung/temperature/state", temperatureString);
}

void mqttSendAdebarLueftersteuerungFanPercent(boolean full)
{
  if (full)
  {
    // see https://www.home-assistant.io/integrations/sensor.mqtt/
    JsonDocument discoveryConfig;
    discoveryConfig["name"] = "Lueftersteuerung Fan Percent";
    discoveryConfig["icon"] = "mdi:fan";
    discoveryConfig["stat_t"] = "adebar/lueftersteuerung/fan_percent/state";
    discoveryConfig["uniq_id"] = "adebar_lueftersteuerung_fan_percent";
    discoveryConfig["state_class"] = "measurement";
    discoveryConfig["unit_of_measurement"] = "%";

    JsonObject device = discoveryConfig["dev"].to<JsonObject>();
    device["identifiers"][0] = "adebar_lueftersteuerung";
    device["name"] = "Lueftersteuerung";

    char buffer[512];
    serializeJson(discoveryConfig, buffer);
    mqttPublish("homeassistant/sensor/adebar_lueftersteuerung_fan_percent/config", buffer);
  }

  char fanPercentString[16];
  snprintf(
      fanPercentString,
      sizeof(fanPercentString),
      "%d",
      fanPercent);

  mqttPublish("adebar/lueftersteuerung/fan_percent/state", fanPercentString);
}

void mqttSendAdebarLueftersteuerungRestartButton(boolean full)
{
  if (!full)
    return;

  // see https://www.home-assistant.io/integrations/button.mqtt/
  JsonDocument discoveryConfig;
  discoveryConfig["name"] = "Lueftersteuerung Neustart";
  discoveryConfig["uniq_id"] = "adebar_lueftersteuerung_system_restart";
  discoveryConfig["cmd_t"] = "adebar/lueftersteuerung/system/set";
  discoveryConfig["payload_press"] = "RESTART";

  JsonObject device = discoveryConfig["dev"].to<JsonObject>();
  device["identifiers"][0] = "adebar_lueftersteuerung";
  device["name"] = "Lueftersteuerung";

  char buffer[256];
  serializeJson(discoveryConfig, buffer);
  mqttPublish("homeassistant/button/adebar_lueftersteuerung_system_restart/config", buffer);
}

void mqttSendStatus(boolean full)
{

  mqttSendAdebarLueftersteuerungIpAddress(full);
  mqttSendAdebarLueftersteuerungRestartButton(full);
  mqttSendAdebarLueftersteuerungTemperature(full);
  mqttSendAdebarLueftersteuerungFanPercent(full);

  lastMqttStatusUpdate = now;
}

bool connectMQTT()
{
  if (mqttClient.connected())
    return true;

  if (now - lastMqttReconnect < MQTT_RECONNECT_TIME)
    return false;

  lastMqttReconnect = now;

  if (mqttClient.connect("lueftersteuerung-esp8266", mqttUser, mqttPassword))
  {
    mqttClient.subscribe("adebar/lueftersteuerung/+/set");
    mqttClient.publish("adebar/lueftersteuerung/system/state", "connected");
    mqttSendStatus(true);

    return true;
  }
  else
  {
    Serial.print("Fehler beim MQTT-Verbindungsversuch: ");
    Serial.println(mqttClient.state());
    return false;
  }
}

// ----------------------------------------------------------
// OTA
// ----------------------------------------------------------
void setupOTA()
{
  ArduinoOTA.setHostname("lueftersteuerung-esp8266");
  ArduinoOTA.setPassword(otaPassword);
  ArduinoOTA.begin();
}

// ----------------------------------------------------------
// SETUP
// ----------------------------------------------------------
void setup()
{
  now = 0;

  Serial.begin(115200);

  now = millis();

  // WLAN und MQTT vorbereiten
  WiFi.mode(WIFI_STA);
  mqttClient.setBufferSize(1024);
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  // Over the Air updates
  setupOTA();

  pinMode(A0, INPUT);
  pinMode(D6, OUTPUT);
}

// ----------------------------------------------------------
// LOOP
// ----------------------------------------------------------
void loop()
{
  // Aktuelle Zeit nur einmal berechnen
  now = millis();

  // WLAN und MQTT-Stuff
  if (connectWifiNonBlocking())
  {
    ArduinoOTA.handle();
    if (connectMQTT())
    {
      mqttClient.loop();

      if (now - lastMqttStatusUpdate >= MQTT_STATUS_INTERVAL)
        mqttSendStatus(true);
    }
  }
  if (now - lastTemperaturCheck >= TEMPERATUR_CHECK_INTERVAL)
  {
    lastTemperaturCheck = now;

    float temperature = readTemperature();
    if (lastTemperature != temperature)
    {
      lastTemperature = temperature;
      Serial.print("Temperatur: ");
      Serial.println(temperature);
      Serial.println(analogRead(A0));
      mqttSendAdebarLueftersteuerungTemperature(false);

      if (!fanPercentManual)
      {
        if (temperature < 19.0)
          fanPercent = 0;
        else if (temperature < 22.0)
          fanPercent = 70;
        else if (temperature < 24.0)
          fanPercent = 90;
        else
          fanPercent = 100;
        mqttSendAdebarLueftersteuerungFanPercent(false);
        analogWrite(D6, fanPercent * 255 / 100);
      }
    }
  }
}
