
#include <SimpleDHT.h>

#include "common.h"
#include "dht.h"

int pinDHT = 2;
SimpleDHT22 dht;

int readSensor(struct SensorData *data) {

  int err = SimpleDHTErrSuccess;
  if ((err = dht.read2(pinDHT, &(data->temp), &(data->hum), NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT failed: " + String(err)) ;
    return 1;
  }
  return 0;
}
  

