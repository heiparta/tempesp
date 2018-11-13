#ifndef __CONFIG_H__
#define __CONFIG_H__

// Sensor type
//#define USE_DHT 1
#define USE_DS18B20 1
#define SENSORPIN 2

// ADC reading error
//#define VOLTAGE_CALIBRATION 0.965
// makuuhuone
//#define VOLTAGE_CALIBRATION 0.9155128205128205
// soljan huone
#define VOLTAGE_CALIBRATION 0.9408895084985501

#define UNITNAME String("soljan_huone")

#endif

