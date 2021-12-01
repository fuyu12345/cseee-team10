// "InitWifi()" Adapted from https://github.com/martinius96/ESP32-eduroam to work at UCL

#include <WiFi.h>
#include "esp_wpa2.h"   // wpa2 for connection to enterprise networks (eduroam)
#include <ThingsBoard.h>

// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// Credentials for tb
#define TOKEN               "0exNgzuDnwPw7iaUumqi"
#define THINGSBOARD_SERVER  "demo.thingsboard.io"

WiFiClient espClient;
ThingsBoard tb(espClient);

int status = WL_IDLE_STATUS;

// Wifi Login
#define EAP_IDENTITY "zcabdgj@ucl.ac.uk"                
#define EAP_PASSWORD "dycdiz-nyMbi1-jopjyz"

const char* essid = "eduroam";

void setup();
void loop();
void InitWifi();

float getCurrentTemperature();
float getCurrentPH();

int getCurrentRPM();

float currentTemperature = 0;
float currentPH = 0;
int currentRPM = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Made it here!");
  InitWifi();
}

// Processes function for RPC call "example_set_temperature"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processTemperatureChange(const RPC_Data &data){
  Serial.println("Received the set temperature RPC method");

  // Process data

  float example_temperature = data["temp"];

  Serial.print("Example temperature: ");
  Serial.println(example_temperature);

  // Just an response example
  return RPC_Response("example_response", 42);
}

// Processes function for RPC call "example_set_switch"
// RPC_Data is a JSON variant, that can be queried using operator[]
// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
RPC_Response processSwitchChange(const RPC_Data &data){
  Serial.println("Received the set switch method");

  // Process data

  bool switch_state = data["switch"];

  Serial.print("Example switch state: ");
  Serial.println(switch_state);

  // Just an response example
  return RPC_Response("example_response", 22.02);
}


const size_t callbacks_size=2;
RPC_Callback callbacks[callbacks_size] = {
  {"example_set_temperature", processTemperatureChange},
  {"example_set_switch", processSwitchChange}  
};

bool subscribed = false;

void loop(){
  // Reconnect to ThingsBoard, if needed
  if (!tb.connected()) {
    subscribed = false;
    
    // Connect to the ThingsBoard 
    Serial.print("Connecting to: "); 
    Serial.print(THINGSBOARD_SERVER); 
    Serial.print(" with token "); 
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    } 
  }

  if (!subscribed){
    Serial.println("Subscribing for RPC...");
    // Perform a subscription. ALl consequent data will happen in
    // processTemperatureChange() and processSwitchChange() functions,
    // as denoted by calbacks[] array.
    if (!tb.RPC_Subscribe(callbacks, callbacks_size)){
      Serial.println("Failed to subscribed for RPC");
      return;  
    }  
    Serial.println("Subscribe done");
    subscribed = true;
  }
  Serial.println("Subscribe done");
  
  currentTemperature = getCurrentTemperature();
  currentPH = getCurrentPH();
  currentRPM = getCurrentRPM();
  
  // Uploads new telemetry to ThingsBoard using MQTT.
  // See https://thingsboard.io/docs/reference/mqtt-api/#telemetry-upload-api
  // for more details
  tb.sendTelemetryFloat("temperature", currentTemperature);
  tb.sendTelemetryFloat("pH", currentPH);
  tb.sendTelemetryInt("stirring", currentRPM);
  // Process messages
  tb.loop();
}

//Random Data
float getCurrentTemperature(){
  float temperature = ((float)rand()/(float)(RAND_MAX)) * 60;
  return temperature;
}

float getCurrentPH(){
  float ph = ((float)rand()/(float)(RAND_MAX)) * 14;
  return ph;
}

int getCurrentRPM(){
  int rpm = random(2000);
  return rpm;
}

void InitWifi(){
  bool eduroamFound = false;
  delay(10);
  // Scan available WiFi networks until eduroam is seen
  
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Repeatedly scan until we see eduroam
  //
  while (!eduroamFound) {
    Serial.println("scan start");
    int n = WiFi.scanNetworks(); // WiFi.scanNetworks returns the number of networks found
    Serial.println("scan done");
    
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            int    rssi = WiFi.RSSI(i);
          
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(ssid);
            Serial.print(" (");
            Serial.print(rssi);
            Serial.print(")");
            Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
            
            ssid.trim();
            if (ssid == essid) {
              Serial.print(" <==== eduroam found");
              eduroamFound = true;
            }
            Serial.println("");
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    if (!eduroamFound)
      delay(5000);
  }
  
  // If we come here, we've successfully found eduroam. Try to connect to it.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(essid);
  
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // This is where the wpa2 magic happens to allow us to connect to eduroam
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  
  WiFi.begin(essid);       //connect to eduroam
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi is connected to ");
  Serial.println(essid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); //print LAN IP
}