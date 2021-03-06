 #include "config.h"
#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}


#include "common.h"
#include "secrets.h"

#ifdef USE_DHT
  #include "dht.h"
#endif
#ifdef USE_DS18B20
  #include "ds18b20.h"
#endif

struct Uptime {
  unsigned long lastMicros;
  unsigned long overflows;
} _uptime;

SensorData oldData;
SensorData newData;
Uptime uptime;
unsigned long nextSend = 0;
uint32_t sendAfterSleeps = 0;
uint32_t failures = 0;
String resetReason = ESP.getResetReason();

int connectWifi();
int readDht(struct SensorData *data);
int sendMeasurement(struct SensorData *data);
unsigned long uptimeSeconds(struct Uptime *up);


int connectWifi() {
  // IPAddress ip(192, 168, 71, 28);
  // IPAddress gateway(192, 168, 71, 1);
  // IPAddress netmask(255, 255, 255, 0);
  // WiFi.config(ip, gateway, netmask);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SID, WIFI_KEY);
  Serial.print("Connecting");
  int maxRetries = 120;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (!maxRetries) {
      return 1;
    }
    delay(500);
    Serial.print(".");
    maxRetries--;
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  return 0;
}


const float BATTERY_DIVIDER = 5.0; // Division done by voltage divider circuit
float readBattery() {
  float vbat = analogRead(A0) * BATTERY_DIVIDER * VOLTAGE_CALIBRATION / 1000;
  Serial.println("Battery voltage" + String(vbat));
  return vbat;
}


int sendInfluxDb(struct SensorData *data) {
  WiFiClient client;
  const char* host = "192.168.8.64";
  if (client.connect(host, 8086))
  {
    // we are connected to the host!
    Serial.println("Connected to host");
  }
  else
  {
    // connection failure
    Serial.println("Connection error");
    return 1;
  }

  String query = "?db=dbtemp";
  String params = "tempdata,host=" + String(UNITNAME) + " temp=" + String(data->temp) + ",vbat=" + String(data->vbat);
  if (data->hum >= 0) {
    params += ",hum=" + String(data->hum);
  }
  if (resetReason != "Deep-Sleep Wake") {
    params += ",status=\"reset\"";
  }
  client.print(String("POST /write") + query + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Connection: close\r\n" +
             "Content-Length: " + String(params.length()) + "\r\n" +
             "\r\n" + 
             params
            );

  while (client.connected()) {
    if (client.available())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  client.stop();
  return 0;
}

int sendThingspeak(struct SensorData *data) {
  WiFiClient client;
  const char* host = "api.thingspeak.com";
  if (client.connect(host, 80))
  {
    // we are connected to the host!
    Serial.println("Connected to host");
  }
  else
  {
    // connection failure
    Serial.println("Connection error");
    return 1;
  }

  // Send over HTTP to api.thingspeak.com
  String params = "?apikey=" + String(TS_API_KEY) + "&field1=" + String(data->temp) + "&field2=" + String(data->hum) + "&field3=" + String(data->vbat);
  if (resetReason != "Deep-Sleep Wake") {
    params += "&status=reset";
  }
  client.print(String("GET /update") + params + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "Connection: close\r\n" +
             "\r\n"
            );

  while (client.connected()) {
    if (client.available())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  client.stop();
  return 0;
}

unsigned long maxMicros = 4294967295;
unsigned long maxSeconds = maxMicros / 1000000;
unsigned long uptimeSeconds(struct Uptime *up) {
  unsigned long seconds = up->lastMicros / 1000000;
  seconds += up->overflows * maxSeconds;
  return seconds;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Started ESP");

  Serial.println("Reset reason: " + resetReason);

  memset(&oldData, 0, sizeof(SensorData));
  if (resetReason == "Deep-Sleep Wake") {
    ESP.rtcUserMemoryRead(0, &sendAfterSleeps, sizeof(sendAfterSleeps));
    ESP.rtcUserMemoryRead(1, (uint32_t *)&oldData, sizeof(oldData));
    ESP.rtcUserMemoryRead(2, &failures, sizeof(failures));

  }

  Serial.println("Sleep cycles left:" + String(sendAfterSleeps));
  Serial.println("Old temp:" + String(oldData.temp));

  memset(&newData, 0, sizeof(SensorData));
  newData.hum = -42;
  memset(&uptime, 0, sizeof(Uptime));

  initSensor();
}

int ALWAYS_DISCONNECT = 1;

int doLoop() {
    unsigned long up = micros();
    if (uptime.lastMicros > up) {
      // Overflow happened
      uptime.overflows++;
    }
    uptime.lastMicros = up;
    unsigned long currentUptime = uptimeSeconds(&uptime);
    Serial.println("Woke up: " + String(currentUptime) + ", Next Send: " + String(nextSend));

    // DHT22 and battery level read are unreliable immediately after reset
    delay(2000);
    newData.vbat = readBattery();
    
    if (readSensor(&newData)) {
      Serial.println("Failed to measure");
      return 1;
    }
    Serial.println("Measured: " + String(newData.temp));
    if (abs(oldData.temp - newData.temp) >= 0.5 || sendAfterSleeps <= 0) {
      // Data changed, try sending it
      if (connectWifi()) {
        Serial.println("Failed to connect to WiFi");
        return 1;
      }
      if (sendInfluxDb(&newData)) {
        Serial.println("Failed to send measurement");
        return 1;
      }
      sendAfterSleeps = 12; // Send every 2 hour at minimum
    }
    oldData = newData;
    sendAfterSleeps--;
    return 0;
}

int sleepTime = 600 * 1000;
//int sleepTime = 60 * 1000;

void doDeepSleep() {
  Serial.println(String("Going to deep sleep: ") + String(sendAfterSleeps) + "/" + String(failures));
  //ESP.rtcUserMemoryWrite(0, &(unsigned int*)oldData, sizeof(oldData));
  ESP.rtcUserMemoryWrite(0, &sendAfterSleeps, sizeof(sendAfterSleeps));
  ESP.rtcUserMemoryWrite(1, (uint32_t *)&oldData, sizeof(oldData));
  ESP.rtcUserMemoryWrite(2, &failures, sizeof(failures));

  ESP.deepSleep(sleepTime * 1000 * (failures ? failures : 1));
}

void loop() {
  int failed = doLoop();

  // Store the number of failures and increase the sleep time up to 10 times the normal sleep time
  if (failed) {
    failures++;
    if (failures > 10) {
      failures = 10;
    }
  } else {
    failures = 0;
  }
  doDeepSleep();
}
