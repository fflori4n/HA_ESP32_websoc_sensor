#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP280.h>

/* One wire pin of DHT22 */
#define DHT_SENS_PIN 25
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

/* One wire pin of DS18B20 */
#define ONE_WIRE_BUS 23
#define TEMPERATURE_PRECISION 12

/* define DHT sensor */
DHT dht(DHT_SENS_PIN, DHTTYPE);

uint16_t sensReadTimer = 4*60000;
uint16_t sendTimer = 4*60000;
double tempSensor_0 = -999.99;
double humiSensor_0 = -999.99;
double tempSensor_1 = -999.99;
/* define DHT sensor */


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideFridge = {0x28, 0xBF, 0x89, 0x80, 0xE3, 0xE1, 0x3C, 0x53};

int8_t readDht(double &temperatureVar, double &humidityVar) {

  double newHumidity = 0, newTemperature = 0;

#define IN_RANGE(VAL, MIN, MAX) ((VAL >= MIN) && (VAL <= MAX))
#define FILTER_COMBINE_READINGS (1.0/6)
#define MAX_DHT_READ_TRY 5

  uint8_t readOKFlag = 0x00;  /* | HUMIDITY READ | TEMP READ |*/

  for (int retry = 0; ((retry < MAX_DHT_READ_TRY) && (readOKFlag != 0x03)); retry++) {

    if ((readOKFlag & 0x01) == 0) {
      newTemperature = dht.readTemperature();
      /*Serial.println(newTemperature);*/
      /*newTemperature = ~(newTemperature & 0x7FFF);*/
      /*Serial.println(newTemperature);*/
      if ((!isnan(newTemperature)) && IN_RANGE(newTemperature, -40.0, 80.0)) {
       /* Serial.println("setting temp.");*/

        if (temperatureVar == -999.99) {
          temperatureVar = newTemperature;
        }
        else {
          temperatureVar = (((1.0 - FILTER_COMBINE_READINGS) * temperatureVar) + ((FILTER_COMBINE_READINGS) * newTemperature));
        }
        readOKFlag |= 0x01;
      }
    }

    if ((readOKFlag & 0x02) == 0) {
      newHumidity = dht.readHumidity();
      if ((!isnan(newHumidity)) && IN_RANGE(newHumidity, 0.0, 100.0)) {

        if (humidityVar == -999.99) {
          humidityVar = newHumidity;
        }
        else {
          humidityVar = (((1.0 - FILTER_COMBINE_READINGS) * humidityVar) + ((FILTER_COMBINE_READINGS) * newHumidity));
        }
        readOKFlag |= 0x02;
      }
    }

  }

#define DBG_DHT_VERBOSE
#ifdef DBG_DHT_VERBOSE
  if (readOKFlag == 0x03) {
    /*Serial.print(F("DHT| [ OK ]"));*/
  }
  else {
    Serial.print(F("DHT| [ ER ]"));
    (readOKFlag == 0) ? Serial.println(F(" Temp & humidity not read.")) : ((readOKFlag == 1) ? Serial.println(F(" Humidity not read.")) : Serial.println(F(" Temp not read.")));
  }
 /* Serial.print(F("Temperature: "));
  Serial.print(temperatureVar);
  Serial.print(F("°C "));
  Serial.print(F("Humidity: "));
  Serial.print(humidityVar);
  Serial.println(F(" %"));*/
#endif

  if (readOKFlag == 0x03) {
    return 0;
  }
  return -1;
}

int8_t readDS18B20(double &temperatureVar){
  /*delay(1000);*/
  sensors.requestTemperatures();
    float newTemperature = sensors.getTempC(insideFridge);
    if (newTemperature == DEVICE_DISCONNECTED_C)
    {
      Serial.println("DS18B20 Error: Could not read temperature data");
      return -1;
    }

    if (temperatureVar == -999.99) {
          temperatureVar = newTemperature;
        }
        else {
          temperatureVar = (((1.0 - FILTER_COMBINE_READINGS) * temperatureVar) + ((FILTER_COMBINE_READINGS) * newTemperature));
        }
Serial.print("Temp C: ");
    Serial.println(temperatureVar);
    return 0;
    
}
