#ifndef __DS18B20_H__
#define __DS18B20_H__
#ifdef USE_DS18B20

#include <OneWire.h>
#include <DallasTemperature.h>

#define SENSORPIN = 2;

int readSensor();


#endif
#endif
