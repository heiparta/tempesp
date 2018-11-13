#ifndef __DHT_H__
#define __DHT_H__
#ifdef USE_DHT

#include "common.h"

int initSensor();
int readSensor(struct SensorData *data);


#endif
#endif
