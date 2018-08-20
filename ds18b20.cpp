#ifdef USE_DS18B20

int readSensor() {

  OneWire oneWire(SENSORPIN);
  DallasTemperature sensors(&oneWire);

  sensors.requestTemperatures();
  int temp = sensors.getTempCByIndex();
  Serial.println("Got temp:" + String(temp));
  return temp;
}
  
#endif
