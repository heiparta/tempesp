#ifndef __DS18B20_H__
#define __DS18B20_H__
#ifdef USE_DS18B20


int initSensor();
int readSensor(struct SensorData *data);

#endif
#endif
