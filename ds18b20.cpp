#include "config.h"
#ifdef USE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

#include "common.h"
#include "ds18b20.h"

OneWire oneWire(SENSORPIN);
DallasTemperature sensors(&oneWire);
  
int initSensor() {
  sensors.begin();
}

int readSensor(struct SensorData *data) {
  sensors.requestTemperatures();
  data->temp = sensors.getTempCByIndex(0);
  return 0;
}
  
#endif
