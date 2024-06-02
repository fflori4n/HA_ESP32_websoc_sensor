typedef struct tWindSensorData {

  const adc1_channel_t rpmHallPin;
  /*const adc1_channel_t rainBucketHallPin;*/
  double windRpm;
  double windGustRpm;
  double windVaneDirection;
  double bme280Temperature;
  double bme280AtmPressure;
  double bme280Humidity;

  boolean windRectangleSignal;

} tWindSensorData;



namespace windSensor {

/* Debug statements */
#define _WINDS_DEBUG_SERIAL Serial
#define _WINDS_DEBUG_OUT_ENABLE 1
/*#define windSens_dbgPrint(WINDS_LOGLVL, WINDS_MSG)    ((_WINDS_DEBUG_OUT_ENABLE != false) && (_WINDS_DEBUG_SERIAL.print(WINDS_MSG)))*/
/*#define windSens_dbgPrintln(...)  ((_WINDS_DEBUG_OUT_ENABLE != false) && (_WINDS_DEBUG_SERIAL.println(__VA_ARGS__)))*/
/* General windSensor stuff */
#define CYCLE_TIME 10

#define MAX_ADC_VALUE 4096;
#define DEBOUNCE_TIME 30

#define ADC_SIG_AVG_FILTER 0.4
#define ADC_BASE_AVG_FILTER 0.002
#define RPM_FILTER 0.002
#define CYCLE_TIME 10
#define MINIMUM_TRIGGERING_DIFFVALUE 70
#define SIGMA_SPIKE_IGNORE 2

#define MULTISAMPLE_ADC 20u
#define SENS_WIND_OFFSET_ANGLE 85.0

#define MAX_COUNTABLE_PERIOD (60000)

#define WIND_RPM_GUST_PERIOD_MS 3000u

#define WINDRPMHALL_TYPICAL_ENCOUNTER_ADC_DIFF 175

/* NOTE: precompute _AvgSamples as 1/numOfSamplesAveraged, to skip division, pass in the correct type or this will not work at all */
#define windSensor_genericExpFilter(_RawValue, _FilteredValue, _AvgSamples, _InitValue) ((_InitValue == _FilteredValue) ? (_RawValue) : (((1 - _AvgSamples) * _FilteredValue) + (_AvgSamples * _RawValue)))

bool rectangleSignal = false;
/* wind rotor RPM */
double rotationPeriod = MAX_COUNTABLE_PERIOD;
double lastRotationPeriod = MAX_COUNTABLE_PERIOD;
double filteredRotationPeriod = MAX_COUNTABLE_PERIOD;
double filteredADC = 0;
double baseFilteredADC = 0;
double avgFastFiltdiff = 0;
uint16_t debounceTimer = 0;
double windRotorRPMFIltered = 0;
/* wind angle sensor*/
uint16_t angleSamplingPeriodCounter = 1000;
uint16_t windRPMUpdatePeriodCounter = WIND_RPM_GUST_PERIOD_MS;
uint16_t windGustUpdatePeriodCounter = 170;
double windAngle = 0;
double filteredWindAngle = -1;

double bmp280Temp = -99;
double bmp280atmPressure = -99;

uint32_t gustMeasurementEnd = 0;
double windGustValue = 0;
double windGustCurrentValue = 0;


boolean debounce(uint16_t* debounceTimer, const uint16_t debouncedAfter) {
  if ((*debounceTimer <= debouncedAfter) && (*debounceTimer < (0xFFFF - CYCLE_TIME))) {
    *debounceTimer += CYCLE_TIME;
    return false;
  }
  return true;
}

void sampleI2CSensors(tWindSensorData& windSensorData, const uint16_t countsBetweenSamples) {

  static uint16_t i2CSensorsSamplePeriodCounter = 0;
  /**/
  if (i2CSensorsSamplePeriodCounter >= countsBetweenSamples) {

    i2CSensorsSamplePeriodCounter = 0;

    double windAngle = (((360.0 / 4095.0) * (double)as5600.readAngle()) + SENS_WIND_OFFSET_ANGLE);
    windAngle = (windAngle >= 360.0) ? (windAngle - 360.0) : (windAngle);

    windSensorData.bme280Temperature = windSensor_genericExpFilter((double)bmp.readTemperature(), (double)windSensorData.bme280Temperature, 0.005, -99.0);
    windSensorData.bme280AtmPressure = windSensor_genericExpFilter((double)bmp.readPressure(), (double)windSensorData.bme280AtmPressure, 0.005, -99.0);
    windSensorData.windVaneDirection =  windSensor_genericExpFilter((double)windAngle, windSensorData.windVaneDirection, 0.005, -1);

    return;

  }

  i2CSensorsSamplePeriodCounter++;
  return;
}

void getData(tWindSensorData& windSensorData) {

  /* sample ADC - multisample for more stable reading */
  double adcValue = adc1_get_raw((windSensorData.rpmHallPin));
  for (int i = 1; i < MULTISAMPLE_ADC; i++) {
    adcValue = 0.9 * adcValue + 0.1 * adc1_get_raw(windSensorData.rpmHallPin);
  }

  filteredADC = windSensor_genericExpFilter( adcValue, filteredADC, ADC_SIG_AVG_FILTER, 0);
  baseFilteredADC = windSensor_genericExpFilter( adcValue, baseFilteredADC, ADC_BASE_AVG_FILTER, 0);

  uint16_t averageAndCurrentDiff = abs(filteredADC - baseFilteredADC);

  if ( averageAndCurrentDiff < (0.3 * WINDRPMHALL_TYPICAL_ENCOUNTER_ADC_DIFF) && debounce(&debounceTimer, DEBOUNCE_TIME)) {
    if (true == rectangleSignal) {
      if (MAX_COUNTABLE_PERIOD < rotationPeriod) {
        lastRotationPeriod = MAX_COUNTABLE_PERIOD;
      }
      else {
        lastRotationPeriod = rotationPeriod;
      }
      filteredRotationPeriod = windSensor_genericExpFilter((double)lastRotationPeriod, (double)filteredRotationPeriod, 0.1, MAX_COUNTABLE_PERIOD);
      rotationPeriod = 0;

    }
    rectangleSignal = false;
  }
  else if (( averageAndCurrentDiff > (0.6 * WINDRPMHALL_TYPICAL_ENCOUNTER_ADC_DIFF)) ) {
    rectangleSignal = true;
    debounceTimer = 0;
  }


  sampleI2CSensors(windSensorData, (1000 / CYCLE_TIME));



  if (windRPMUpdatePeriodCounter >= (WIND_RPM_GUST_PERIOD_MS / CYCLE_TIME)) {
    windRPMUpdatePeriodCounter = 0;
    windRotorRPMFIltered = (MAX_COUNTABLE_PERIOD == filteredRotationPeriod) ? 0 : (60000 / filteredRotationPeriod);

    if (windGustCurrentValue < windRotorRPMFIltered) {
      gustMeasurementEnd = esp_timer_get_time();
      Serial.println(gustMeasurementEnd);
      windGustCurrentValue = windRotorRPMFIltered;
    }

    if (windGustUpdatePeriodCounter >= ((10 * 60) / 3)) {
      windGustUpdatePeriodCounter = 0;
      windGustValue = windGustCurrentValue;
      windGustCurrentValue = 0;
    }
    else {
      windGustUpdatePeriodCounter++;
    }


  }
  else {
    windRPMUpdatePeriodCounter++;
  }

  windSensorData.windGustRpm = windGustValue;
  windSensorData.windRpm = windRotorRPMFIltered;

  /*Serial.print("filtADC:");
    Serial.print(filteredADC);
    Serial.print(",fastFilt:");
    Serial.print(baseFilteredADC);
    Serial.print(",rect:");
    Serial.print(rectangleSignal ? 0 : 1000);
    Serial.print(",tresh_low:");
    Serial.print((0.3 * WINDRPMHALL_TYPICAL_ENCOUNTER_ADC_DIFF) + baseFilteredADC);
    Serial.print(",tresh_high:");
    Serial.print((0.6 * WINDRPMHALL_TYPICAL_ENCOUNTER_ADC_DIFF) + baseFilteredADC);
    Serial.print(",angle:");
    Serial.print(windAngle);*/
  /*Serial.print(",period:");
    Serial.print(lastRotationPeriod);
    Serial.print(",timer:");
    Serial.print(rotationPeriod);*/
  /*Serial.print(",RPM:");
    Serial.println(windRotorRPM);*/



  if ((rotationPeriod < MAX_COUNTABLE_PERIOD) && (rotationPeriod < (0xFFFF - CYCLE_TIME))) {
    rotationPeriod += CYCLE_TIME;
  }
  else {
    rotationPeriod = MAX_COUNTABLE_PERIOD;
    filteredRotationPeriod = MAX_COUNTABLE_PERIOD;
    windRotorRPMFIltered = 0;
  }
}
}
