#include "config.h"

#ifdef USE_DHT

#include <SimpleDHT.h>

#include "common.h"
#include "dht.h"

SimpleDHT22 dht;

int initSensor() {
}

int readSensor(struct SensorData *data) {

  int err = SimpleDHTErrSuccess;
  if ((err = dht.read2(SENSORPIN, &(data->temp), &(data->hum), NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT failed: " + String(err)) ;
    return 1;
  }
  return 0;
}

#endif

