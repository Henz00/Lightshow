/* INCLUDE LIBRARIES HERE */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <certs.h>
#include <ArduinoOTA.h>

/* INCLUDE FILES HERE */
#include <./ledstrip/led_strip.h>

/* -------------------------------------- VARIABLES SECTION BELOW -------------------------- */
/* Supportive data structures */

/* variables for LED setup */
#define TOTAL_LEDS 120
CRGB leds[TOTAL_LEDS];
LEDStrip strip_bottom(&leds[0], 60);
LEDStrip strip_top(&leds[60], 60);
bool LEDLoopRunning = true;
static unsigned long lastLedUpdate = 0;

JsonDocument mode_1;
JsonDocument mode_2;
static unsigned long lastModeUpdate = 0;
bool toggleModeState = true;

/* variables for wifi connection */
char ssid[] = WIFI_SSID;
char password[] = WIFI_PASS;

/* variables for broker(pub,sub) connection */
WiFiClientSecure espClient;
PubSubClient client(espClient);
const char* mqtt_server = MQTT_SERVER_IP;
int mqtt_port = atoi(MQTT_SERVER_PORT);


/* -------------------------------------------------------------------------------------- */

// put function declarations here:
void ConnectToWifi(char[], char[]);
void SetupMQTT(PubSubClient&, WiFiClientSecure&, const char*, int);
void ConnectMQTT(PubSubClient&);
void callback(char*, byte*, unsigned int);
void handleCommand(String);
void timeSync();
void updateLEDStrips(JsonDocument, LEDStrip&);
void updateLEDModeVariable(JsonDocument&, JsonDocument, String);
void InitializeLEDs();


void setup() {
  Serial.begin(115200);

  InitializeLEDs();
  
  ConnectToWifi(ssid, password);

  ArduinoOTA.setHostname("esp8266-lightshow");
  ArduinoOTA.begin();

  delay(1000);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  timeSync();

  SetupMQTT(client, espClient, mqtt_server, mqtt_port);
}

void loop() {

  ConnectMQTT(client);

  client.loop();

  if (millis() - lastModeUpdate > 15000) {
    lastModeUpdate = millis();

    if(toggleModeState){
      updateLEDStrips(mode_1["top"], strip_top);
      updateLEDStrips(mode_1["bottom"], strip_bottom);
      toggleModeState = false;
    } else {
      updateLEDStrips(mode_2["top"], strip_top);
      updateLEDStrips(mode_2["bottom"], strip_bottom);
      toggleModeState = true;
    }

  }
  

  if (millis() - lastLedUpdate > 20) {
    lastLedUpdate = millis();

    if (LEDLoopRunning) {
        strip_bottom.loop();
        strip_top.loop();
        FastLED.show();
    }
  }

  yield();

  ArduinoOTA.handle();
}

// ALL FUNCTION DEFINITIONS:

void ConnectToWifi(char ssid[], char password[]){
  WiFi.begin(ssid, password);
  Serial.println("connecting...");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi connected");
  Serial.println("IP: " + WiFi.localIP().toString());
}

void SetupMQTT(PubSubClient& client, WiFiClientSecure& wifiClient, const char* mqtt_server, int mqtt_server_port){
  static BearSSL::X509List ca(AWS_CERT_CA);
  static BearSSL::X509List cert(AWS_CERT_CRT);
  static BearSSL::PrivateKey key(AWS_CERT_PRIVATE);

  wifiClient.setTrustAnchors(&ca);
  wifiClient.setClientRSACert(&cert, &key);

  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
  client.setKeepAlive(120);

}

void ConnectMQTT(PubSubClient& client){
  static unsigned long lastReconnectAttempt = 0;

  if (!client.connected()) {
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      
      Serial.print("Attempting MQTT connection...");

      if (client.connect("esp8266-office")) {
        Serial.println("connected!");
        
        client.publish("server/logger", "esp: connected");
        client.subscribe("office/ledstrip");
        
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" retrying...");
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message;
  Serial.print("Message: ");
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  Serial.println("");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  String command = doc["command"].as<String>();
  
  updateLEDModeVariable(mode_1, doc["bottom"], "bottom");
  updateLEDModeVariable(mode_1, doc["top"], "top");
  
  lastModeUpdate = 0;
  toggleModeState = true;

  handleCommand(command);
  
  
}

void handleCommand(String command){
  if(command == "start"){
    LEDLoopRunning = true;
  } else if (command == "pause"){
    LEDLoopRunning = false;
  } else if (command == "stop"){
    Serial.println("stopping led");
    LEDLoopRunning = false;
    strip_bottom.resetLEDStrip();
    strip_top.resetLEDStrip();
  } else if (command == "print"){
    // todo
  }
}

void timeSync(){
  Serial.print("Waiting for time sync");

  time_t now = time(nullptr);
  while (now < 100000) {  // arbitrary valid timestamp
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println(" Time synced!");
}

void updateLEDStrips(JsonDocument doc, LEDStrip& strip){
  strip.handleMode(doc["mode"].as<String>());
  strip.handleSpeed(doc["speed"]);
  strip.handleProgress(doc["progress"]);
  strip.handleAnimation(doc["animation"].as<String>());
}

void updateLEDModeVariable(JsonDocument& mode, JsonDocument data, String led_strip){
  mode[led_strip]["mode"] = data["mode"].as<String>();
  mode[led_strip]["speed"] = data["speed"];
  mode[led_strip]["progress"] = data["progress"];
  mode[led_strip]["animation"] = data["animation"].as<String>();
}

void InitializeLEDs(){
  FastLED.addLeds<WS2812B, D4, GRB>(leds, TOTAL_LEDS);

  mode_1["top"]["mode"] = "rainbow";
  mode_1["top"]["speed"] = 40;
  mode_1["top"]["progress"] = 0;
  mode_1["top"]["animation"] = "fill";
  mode_1["bottom"]["mode"] = "rainbow";
  mode_1["bottom"]["speed"] = 40;
  mode_1["bottom"]["progress"] = 0;
  mode_1["bottom"]["animation"] = "fill";

  mode_2["top"]["mode"] = "ambient";
  mode_2["top"]["speed"] = 60;
  mode_2["top"]["progress"] = 0;
  mode_2["top"]["animation"] = "fill";
  mode_2["bottom"]["mode"] = "ambient";
  mode_2["bottom"]["speed"] = 60;
  mode_2["bottom"]["progress"] = 0;
  mode_2["bottom"]["animation"] = "fill";
}
