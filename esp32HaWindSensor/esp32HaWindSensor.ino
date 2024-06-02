#include <Arduino.h>
#include <esp_task_wdt.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "time.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "HAwebsocket.h"
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

#include "AS5600.h"
#include "sensors.h"
#include <SPI.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp(&Wire); // I2C
AS5600L as5600(0x36);   //  use default Wire


#include "windSensor.h"

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

#define AUTH_JSON_PAYLOAD "{\"type\":\"auth\",\"access_token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc\"}"
#define CREATE_PING "{\"id\":%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"ping\", \"state\": 15, \"icon\":\"mdi:lan-connect\", \"friendly_name\":\"Ping\", \"timeout\":\"1\", \"inf\":\"hello world!\"}}"
#define UPDATE_PING "{\"id\":1,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.update\", \"service_data\": {\"name\": \"ping\", \"state\": 12 }}"


#define DHT_TEMP "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"dhtTemp\", \"state\": %d.%d, \"icon\":\"mdi:home-thermometer\", \"friendly_name\":\"dhtTemp\", \"device_class\":\"temperature\",\"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define DHT_HUMI "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"dhtHumi\", \"state\": %d.%d, \"icon\":\"mdi:water-percent\", \"friendly_name\":\"dhtHumi\", \"device_class\":\"humidity\",\"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define DS18B20_TEMP "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"DS18Temp\", \"state\": %d.%d, \"icon\":\"mdi:thermometer\", \"friendly_name\":\"ds18Temp\",\"device_class\":\"temperature\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"

#define PHOTO_RES "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"light\", \"state\": %d.%d, \"icon\":\"mdi:sun-wireless-outline\", \"friendly_name\":\"light\",\"device_class\":\"light\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define BMP280_TEMP "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"BMP280_temp\", \"state\": %d.%d, \"icon\":\"mdi:sun-thermometer-outline\", \"friendly_name\":\"Outside temp\",\"device_class\":\"temperature\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define BMP280_ATM_PRESS "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"BMP280_atmPressure\", \"state\": %d.%d, \"icon\":\"mdi:airballoon\", \"friendly_name\":\"Atm. pressure\",\"device_class\":\"pressure\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"

#define WIND_RPM "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"WindSensor_RPM\", \"state\": %d.%d, \"icon\":\"mdi:weather-windy\", \"friendly_name\":\"Wind RPM\",\"device_class\":\"rpm\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define WIND_GUST_RPM "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"WindSensor_windGust_RPM\", \"state\": %d.%d, \"icon\":\"mdi:weather-windy\", \"friendly_name\":\"Wind Gust RPM\",\"device_class\":\"rpm\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define WIND_ANGLE "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"WindSensor_Direction\", \"state\": %d.%d, \"icon\":\"mdi:windsock\", \"friendly_name\":\"Wind Direction Angle\",\"device_class\":\"angle\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"

#define TEMPLATE_UP_TIME "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"WindSensor_UpTime\", \"state\": %d, \"icon\":\"mdi:counter\", \"friendly_name\":\"Sensor up time\",\"device_class\":\"time\", \"state_class\":\"measurement\",\"timeout\":\"5\"}}"
#define TEMPLATE_REBOOT_REASON "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"RebootCause\", \"state\": \"%s\", \"icon\":\"mdi:counter\", \"friendly_name\":\"Last Reboot Caused by\",\"timeout\":\"0\"}}"
#define TEMPLATE_FREE_HEEP "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.create\", \"service_data\": {\"name\": \"Heep\", \"state\": %d, \"icon\":\"mdi:counter\", \"timeout\":\"0\"}}"

#define TEMPLATE_VALUES_ONLY "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.set_values\", \"service_data\": {%s}}"

#define TEMPLATE_VALUES_ONLY_START_CAT "{\"id\":%%d,\"type\":\"call_service\", \"domain\": \"websoc_sensor\", \"service\":\"wind_sens.set_values\", \"service_data\": {"
#define TEMPLATE_VALUES_ONLY_END_CAT "}}"

#define REPORT_TIME_MINS 1


enum tSoftResetReason{
  tSoftResetReason_Unknown,
  tSoftResetReason_NOCON,
};

static RTC_NOINIT_ATTR uint8_t softResetReasonFlag;

double bMP280AirPressure = 0;
double bMP280Temperature = 0;
double lightIntensityPercent = 0;

double windRpm = 0;
double windGustRPM = 0;
double windAngle = 0;
double windSensTemp = 0;
double windSensAtmPressure = 0;

tWindSensorData windsensorData = {ADC1_CHANNEL_3, -1, -1, -1, -99.0, -99.0, -99.0, false};

uint32_t lastReportSentToHAAt = 0xFFFFFFFF;
uint8_t espLastResetCause = 0;

boolean _wifiConnectionStable = false;  /* PLS write only from wifi task*/


void WIFI_manageConnected() {

#define WIFI_SSID "TS-uG65" 
#define WIFI_PASS "4XfuPgEx" /* LOL this probably shouldnt be on github...*/

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wifi is connected.");
      return;
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("Wifi is connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void WIFI_KeepConnectionTask(void* parargs) {

#define WIFI_SSID "TS-uG65"
#define WIFI_PASS "4XfuPgEx"  /* and defined twice?? */

#define WIFI_TRY_CONNECT_TIME   20000u
#define WIFI_WAIT_TIME          30000u
#define WIFI_NOCON_RESET_AFTER  5u

uint8_t con = 0;
uint8_t noNetworkResetCounter = 0;
uint16_t connectionStableCounter = 0;

softResetReasonFlag = tSoftResetReason_Unknown;

  for (;;) {

    if (WiFi.status() != WL_CONNECTED) {

      connectionStableCounter = 0;
      _wifiConnectionStable = false;

      WiFi.mode(WIFI_STA);

      Serial.println("Wifi is connecting...");
      
      for(; ((con < (WIFI_TRY_CONNECT_TIME/1000)) && (WiFi.status() != WL_CONNECTED)); con++){
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.print(".");
      }

      if(con >= (WIFI_TRY_CONNECT_TIME/1000)){
        Serial.println("Wifi connection failed, will reset and retry in 30sec...");
        vTaskDelay((WIFI_WAIT_TIME)/ portTICK_PERIOD_MS);
        noNetworkResetCounter++;
        if(noNetworkResetCounter >=  WIFI_NOCON_RESET_AFTER){
          softResetReasonFlag = tSoftResetReason_NOCON;
          esp_restart();
        }
      }  
      else{
        Serial.println("Connected [OK]");
      }
    }
    else{
      if(connectionStableCounter >= 60000){
        _wifiConnectionStable = true;
      }
      else{
        connectionStableCounter+= 10000;
      }
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
  }
}

/* 1 normal power on*/
/* 6 Watch dog timer reset */
/* 7 Recovered from brownout */
char* getEspLastResetReason(uint8_t reason)
{

  if(tSoftResetReason_NOCON == softResetReasonFlag){
    return "SoftReset_NoWifiCon";
  }
  switch (reason)
  {
    case 0 : return "Unknown"; break;
    case 1 : return "Power_On"; break;         /**<1, Vbat power on reset*/
    case 3 : return "SW_RESET"; break;              /**<3, Software reset digital core*/
    case 4 : return "OWDT_RESET"; break;            /**<4, Legacy watch dog reset digital core*/
    case 5 : return "DEEPSLEEP_RESET"; break;       /**<5, Deep Sleep reset digital core*/
    case 6 : return "SDIO_RESET"; break;            /**<6, Reset by SLC module, reset digital core*/
    case 7 : return "TG0WDT_SYS_RESET"; break;      /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : return "TG1WDT_SYS_RESET"; break;      /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : return "RTCWDT_SYS_RESET"; break;      /**<9, RTC Watch dog Reset digital core*/
    case 10 : return "INTRUSION_RESET"; break;      /**<10, Instrusion tested to reset CPU*/
    case 11 : return "TGWDT_CPU_RESET"; break;      /**<11, Time Group reset CPU*/
    case 12 : return "SW_CPU_RESET"; break;         /**<12, Software reset CPU*/
    case 13 : return "RTCWDT_CPU_RESET"; break;     /**<13, RTC Watch dog Reset CPU*/
    case 14 : return "EXT_CPU_RESET"; break;        /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return "RTCWDT_BROWN_OUT_RESET"; break; /**<15, Reset when the vdd voltage is not stable*/
    case 16 : return "RTCWDT_RTC_RESET"; break;     /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return "Unknown";
  }
}


#define MAIN_CYCLE_MS 10






HAWebsocClient homeassistantClient;

#define _TEMP_STRLEN 2048
char tempStrBuffer[_TEMP_STRLEN] = {0};
char keyValueStr[_TEMP_STRLEN] = {0};
double uptimeHours = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  esp_task_wdt_init((60 * (3 * REPORT_TIME_MINS)), true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

  espLastResetCause = esp_reset_reason();
  Serial.println(getEspLastResetReason(espLastResetCause));
  Wire.setPins(21, 22);
  Wire.begin(21, 22);

  /* xTaskCreatePinnedToCore(
     WIFI_manageConnected,
     "manageWifi",
     5000,
     NULL,
     1,
     NULL,
     CONFIG_ARDUINO_RUNNING_CORE
     );*/
  /*WIFI_manageConnected();*/

  dht.begin();

  /* init DS18B20 sensors */
  sensors.begin();

  /* print DS18B20 sensors that are present on bus */
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");


  if (!sensors.getAddress(insideFridge, 0)) Serial.println("Unable to find address for Device 0");
  /* set precision of DS18B20 */
  sensors.setResolution(insideFridge, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideFridge), DEC);
  Serial.println();

  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76, 0x58);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                     "try a different address!"));
    Serial.print("BMP SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  as5600.begin(4);  //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
  int b = as5600.isConnected();
  Serial.print("Connect: ");
  Serial.println(b);

  esp_task_wdt_reset();

  xTaskCreatePinnedToCore(WIFI_KeepConnectionTask,"KeepCon",5000,NULL,1,NULL,CONFIG_ARDUINO_RUNNING_CORE);
}

char getDigit(int32_t value, int digit, bool useSpace = false){
  
      if(digit == -1){ /// return sign
        if(value < 0){
          return '-';
        }
        return ' ';
      }
      
      return '0' + (abs(value)%((int)pow(10,(digit + 1))))/ ((int)pow(10, digit));
    }
    
double measureLightIntensity_34() {
#define MEASURE_LIGHT_INTENSITY_AVGFILTCONST (0.02)
#define MEASURE_LIGHT_INTENSITY_ADC_TO_PERCENT ((100.0/4095))

  static double filteredLightIntensity = 0;

  double adcReading = analogRead(36);

  if ( 0 == filteredLightIntensity ) {
    filteredLightIntensity = (adcReading * MEASURE_LIGHT_INTENSITY_ADC_TO_PERCENT);
  }
  else {
    filteredLightIntensity = ((1 - MEASURE_LIGHT_INTENSITY_AVGFILTCONST) * filteredLightIntensity) + ((MEASURE_LIGHT_INTENSITY_AVGFILTCONST) * (adcReading * MEASURE_LIGHT_INTENSITY_ADC_TO_PERCENT));
  }
  return (100 - filteredLightIntensity);
}

void readBMP280(double& temp, double& atmPressure) {

#define BMP280_TEMP_AVGFILTCONST (0.02)
#define BMP280_AIRP_AVGFILTCONST (0.01)

  static double filteredTemp = -99;
  static double filteredAtmPressure = -99;

  if ( -99 == filteredTemp ) {
    filteredTemp = bmp.readTemperature();
  }
  else {
    filteredTemp = ((1 - BMP280_TEMP_AVGFILTCONST) * filteredTemp) + ((BMP280_TEMP_AVGFILTCONST) * bmp.readTemperature());
  }

  if ( -99 == filteredAtmPressure ) {
    filteredAtmPressure = (bmp.readPressure() / 100.0);
  }
  else {
    filteredAtmPressure = ((1 - BMP280_AIRP_AVGFILTCONST) * filteredAtmPressure) + ((BMP280_AIRP_AVGFILTCONST) * (bmp.readPressure() / 100.0));
  }

  temp = filteredTemp;
  atmPressure = filteredAtmPressure; /* convert it to mBar */
}

void loop() {
  char jsonBuffer[2048] = {0};
  uint32_t  esp_timer_now = esp_timer_get_time() / 1000;
  uint32_t  uptimeSeconds = esp_timer_get_time() / 1000 / 1000;
  uint32_t uptimeMinutes = 0;
  /* Check if it is time to read DHT and DS18B20 sensors */
  windSensor::getData(windsensorData);

  if ((sensReadTimer * MAIN_CYCLE_MS) >= 1000) {
    /*Serial.print("reading dht");*/

    if (/*readDht(tempSensor_0, humiSensor_0) == 0 && */readDS18B20(tempSensor_1) == 0) {
      sensReadTimer = 0;
    }

    lightIntensityPercent = 100.0 - measureLightIntensity_34();


    /* readBMP280(bMP280Temperature, bMP280AirPressure);*/

    /*Serial.print("BMP280 Atm Pressure: ");
      Serial.println(bMP280AirPressure);
      Serial.print("BMP280 Temp: ");
      Serial.println(bMP280Temperature);*/
    Serial.print("Uptime:");
    Serial.print(uptimeSeconds);
    Serial.print(" ,light: ");
    Serial.println(lightIntensityPercent);
    Serial.print(" Wind rpm:");
    Serial.print(windsensorData.windRpm);
    Serial.print(" ,windGustRPM:");
    Serial.print(windsensorData.windGustRpm);
    Serial.print(",rotation:");
    Serial.print(windsensorData.windVaneDirection);
    Serial.print(",atm pressure:");
    Serial.print(windsensorData.bme280AtmPressure);
    Serial.print(",temp:");
    Serial.println(windsensorData.bme280Temperature);
    Serial.print("heep:");
    Serial.println(ESP.getFreeHeap());
    Serial.print("next report in: ");
    Serial.println(((REPORT_TIME_MINS * 60000) - ((esp_timer_now - lastReportSentToHAAt))));
  }
  else {
    sensReadTimer++;
  }

  if (((esp_timer_now - lastReportSentToHAAt) > (REPORT_TIME_MINS * 60000)) || (0xFFFFFFFF == lastReportSentToHAAt)) {
    
    if (true == _wifiConnectionStable) {

      lastReportSentToHAAt = esp_timer_now;
      
      homeassistantClient.connectAndAuthSocket();
      vTaskDelay(50 / portTICK_PERIOD_MS);
      homeassistantClient.pingPong();
      vTaskDelay(50 / portTICK_PERIOD_MS);

      /* snprintf(jsonBuffer, 2048, DS18B20_TEMP, (int16_t)(tempSensor_1), ((int16_t)(tempSensor_1 * 100)) % 100);
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);
        snprintf(jsonBuffer, 2048, PHOTO_RES, (int16_t)(lightIntensityPercent), ((int16_t)abs((lightIntensityPercent * 100)) % 100));
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);*/
      /*snprintf(jsonBuffer, 2048, BMP280_ATM_PRESS, (int16_t)((windsensorData.bme280AtmPressure/100)), ((int16_t)abs(((windsensorData.bme280AtmPressure/100) * 100)) % 100));
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);
        snprintf(jsonBuffer, 2048, BMP280_TEMP, (int16_t)(windsensorData.bme280Temperature), ((int16_t)abs((windsensorData.bme280Temperature * 100)) % 100));
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);

        snprintf(jsonBuffer, 2048, WIND_RPM, (int16_t)(windsensorData.windRpm), ((int16_t)(windsensorData.windRpm * 100)) % 100);
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);
        snprintf(jsonBuffer, 2048, WIND_GUST_RPM, (int16_t)(windsensorData.windGustRpm), ((int16_t)(windsensorData.windGustRpm * 100)) % 100);
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);
        snprintf(jsonBuffer, 2048, WIND_ANGLE, (int16_t)(windsensorData.windVaneDirection), ((int16_t)(windsensorData.windVaneDirection * 100)) % 100);
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);

        double uptimeMinutes = ((double)uptimeSeconds)/60;
        snprintf(jsonBuffer, 2048, TEMPLATE_UP_TIME, ((int16_t)(uptimeMinutes)), ((int16_t)(uptimeSeconds * 100)) % 100);
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        delay(1000);*/

      /* snprintf(jsonBuffer, 2048, TEMPLATE_REBOOT_REASON, getEspLastResetReason(espLastResetCause));
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);
        espLastResetCause = 0;
        delay(1000);

        snprintf(jsonBuffer, 2048, TEMPLATE_FREE_HEEP, ESP.getFreeHeap());
        homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL,2000,5);*/

      uptimeMinutes = ((double)uptimeSeconds) / 60;
      tempStrBuffer[HAWEBSOC_CLI_OUTBUFFER_SIZE] = {0};

      snprintf(jsonBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, TEMPLATE_VALUES_ONLY_START_CAT);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"DS18Temp\":%d.%d,", (int16_t)(tempSensor_1), ((int16_t)(tempSensor_1 * 100)) % 100);
      strcat(jsonBuffer, tempStrBuffer);
      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"light\":%d.%d,", (int16_t)(lightIntensityPercent), ((int16_t)abs((lightIntensityPercent * 100)) % 100));
      strcat(jsonBuffer, tempStrBuffer);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"BMP280_temp\":%d.%d,", (int16_t)(windsensorData.bme280Temperature), ((int16_t)abs((windsensorData.bme280Temperature * 100)) % 100));
      strcat(jsonBuffer, tempStrBuffer);
      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"BMP280_atmPressure\":%d.%d,", (int16_t)((windsensorData.bme280AtmPressure / 100)), ((int16_t)abs(((windsensorData.bme280AtmPressure / 100) * 100)) % 100));
      strcat(jsonBuffer, tempStrBuffer);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"WindSensor_RPM\":%d.%d,", (int16_t)(windsensorData.windRpm), ((int16_t)(windsensorData.windRpm * 100)) % 100);
      strcat(jsonBuffer, tempStrBuffer);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"WindSensor_windGust_RPM\":%d.%d,", (int16_t)(windsensorData.windGustRpm), ((int16_t)(windsensorData.windGustRpm * 100)) % 100);
      strcat(jsonBuffer, tempStrBuffer);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"WindSensor_Direction\":%d.%d,", (int16_t)(windsensorData.windVaneDirection), ((int16_t)(windsensorData.windVaneDirection * 100)) % 100);
      strcat(jsonBuffer, tempStrBuffer);

      uint32_t uptimeHours = (uint32_t)((((double)uptimeMinutes)/60.0)*100);

      
      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"WindSensor_UpTime\":%d.%d%d,", uptimeHours/100, (uptimeHours%100)/10, (uptimeHours%10));
      strcat(jsonBuffer, tempStrBuffer);

      snprintf(tempStrBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, "\"RebootCause\":\"%s\"", getEspLastResetReason(espLastResetCause));
      strcat(jsonBuffer, tempStrBuffer);

      strcat(jsonBuffer, TEMPLATE_VALUES_ONLY_END_CAT );
      /*snprintf(jsonBuffer, 2048, TEMPLATE_VALUES_ONLY, keyValueStr);*/
      homeassistantClient.writeWithRetry(jsonBuffer, 5000, "\"success\":true", NULL, 2000, 5);

      esp_task_wdt_reset();
    }
    else{
      esp_task_wdt_reset();
    }
    
  }
  vTaskDelay((MAIN_CYCLE_MS) / portTICK_PERIOD_MS);
}
