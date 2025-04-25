/*************************************************** 
  SHT4x Humidity & Temp Sensor:       https://www.adafruit.com/product/5665
  Adafruit 128x64 OLED FeatherWing:   https://www.adafruit.com/product/4650
  Adafruit Feather M4 Express:        https://www.adafruit.com/product/3857
  Adafruit SGP40 Air Quality Sensor:  https://www.adafruit.com/product/4829

  Adafruit BMP390 - Precision Barometric Pressure and Altimeter  https://www.adafruit.com/product/4816
  Adafruit SCD-41 - True CO2 Temperature and Humidity Sensor     https://www.adafruit.com/product/5190
  STEMMA QT / Qwiic JST SH 4-Pin Cable - 50mm Long               https://www.adafruit.com/product/4399
 
  Use buttons to display sensor serial numbers.
  SHT4x
  SGP4x  

  Arduino SAMD Boards
  https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

  Adafruit SHT4x Library
  Adafruit SSD1306
  Adafruit Unified Sensor
  Adafruit BusIO
  Adafruit GFX Library
  Adafruit SH110X
  Adafruit SGP40 Sensor
  Adafruit BMP3XX
  Sensirion I2C SCD4x  
 ****************************************************/

#include <Adafruit_SHT4x.h>
#include "Adafruit_SGP40.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <math.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
// #include <Arduino.h>
#include <SensirionI2cScd4x.h>

#define VBATPIN A6
// #define SEALEVELPRESSURE_HPA (1013.25)
// https://forecast.weather.gov/data/obhistory/KMLB.html

float measuredBatteryVoltage = 0.0;
float t, h = 0.0;
double heat_index = 0.0;
double dew_point = 0.0;
double alpha = 0.0;
double a = 17.625;
double b = 243.04;
double pressure = 0.0;
int count = 0;

// Originally declared in the loop.
bool dataReady = false;
uint16_t scd4_co2Concentration = 0;
float scd4_temperature = 0.0;
float scd4_relativeHumidity = 0.0;

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_SGP40 sgp = Adafruit_SGP40();
Adafruit_BMP3XX bmp;

// SCD4x Section
// #include <Arduino.h>
// #include <SensirionI2cScd4x.h>
// #include <Wire.h>

// macro definitions
// make sure that we use the proper definition of NO_ERROR
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

SensirionI2cScd4x scd;

static char errorMessage[64];
static int16_t error;

void PrintUint64(uint64_t& value) {
    Serial.print("0x");
    Serial.print((uint32_t)(value >> 32), HEX);
    Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}


void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(100); } // Wait for serial console to open!
  // delay(1000);

  Serial.println("SHT4x, SGP40, BMP3xx, SCD4x, and 128x64 OLED Weather Station");


  display.begin(0x3C, true); // Address 0x3C default
  if (! display.begin()) {
    Serial.println("Couldn't find OLED");
    while (1) delay(1);
  }
  Serial.println("OLED Begun");

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  // display.display();
  // delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  // display.display();

  display.setRotation(1);
  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.display(); // actually display all of the above
  delay(10);  // Move above display.display(); to stop start glitch?


  if (! sgp.begin()){
    Serial.println("SGP40 sensor not found :(");
    while (1);
  }

  Serial.print("Found SGP40 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);


  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  switch (sht4.getPrecision()) {
     case SHT4X_HIGH_PRECISION: 
       Serial.println("High precision");
       break;
     case SHT4X_MED_PRECISION: 
       Serial.println("Med precision");
       break;
     case SHT4X_LOW_PRECISION: 
       Serial.println("Low precision");
       break;
  }

  // You can have 6 different heater settings
  // higher heat and longer times uses more power
  // and reads will take longer too!
  sht4.setHeater(SHT4X_NO_HEATER);
  switch (sht4.getHeater()) {
     case SHT4X_NO_HEATER: 
       Serial.println("No heater");
       break;
     case SHT4X_HIGH_HEATER_1S: 
       Serial.println("High heat for 1 second");
       break;
     case SHT4X_HIGH_HEATER_100MS: 
       Serial.println("High heat for 0.1 second");
       break;
     case SHT4X_MED_HEATER_1S: 
       Serial.println("Medium heat for 1 second");
       break;
     case SHT4X_MED_HEATER_100MS: 
       Serial.println("Medium heat for 0.1 second");
       break;
     case SHT4X_LOW_HEATER_1S: 
       Serial.println("Low heat for 1 second");
       break;
     case SHT4X_LOW_HEATER_100MS: 
       Serial.println("Low heat for 0.1 second");
       break;
  }


  if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
  //if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
  //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);


  scd.begin(Wire, SCD41_I2C_ADDR_62);

  uint64_t serialNumber = 0;
  delay(30);
  // Ensure sensor is in clean state
  error = scd.wakeUp();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute wakeUp(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  error = scd.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  error = scd.reinit();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute reinit(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
  }
  // Read out information about the sensor
  error = scd.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  Serial.print("serial number: ");
  PrintUint64(serialNumber);
  Serial.println();
  //
  // If temperature offset and/or sensor altitude compensation
  // is required, you should call the respective functions here.
  // Check out the header file for the function definitions.
  // Start periodic measurements (5sec interval)
  error = scd.startPeriodicMeasurement();
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  //
  // If low-power mode is required, switch to the low power
  // measurement function instead of the standard measurement
  // function above. Check out the header file for the definition.
  // For SCD41, you can also check out the single shot measurement example.
  //
  
  // delay(5000); // Wait 5 Seconds for the SCD4x sensor. 

}


void loop() {
  uint32_t timestamp = millis();

  sensors_event_t humidity, temp;
  uint16_t sraw;
  int32_t voc_index;
  
  sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data

  t = temp.temperature;
  h = humidity.relative_humidity;

  // Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  // Serial.print("Hum. % = "); Serial.println(h);


  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  pressure = bmp.pressure;
  

  sraw = sgp.measureRaw(t, h);
  // Serial.print("Raw measurement: ");
  // Serial.println(sraw);

  voc_index = sgp.measureVocIndex(t, h);
  // Serial.print("Voc Index: ");
  // Serial.println(voc_index);

  // Serial.print("Temperature: "); Serial.print(t); Serial.println(" degrees C");
  // Serial.print("Humidity: "); Serial.print(h); Serial.println("% rH");

  // Read the battery voltage. 
  measuredBatteryVoltage = analogRead(VBATPIN);
  measuredBatteryVoltage *= 2;    // Multiply by 2 because of the resistive divide by 2. 
  measuredBatteryVoltage *= 3.3;  // Multiply by the analog reference voltage of 3.3V.
  measuredBatteryVoltage /= 1024; // Divide by 1024 to convert to a voltage.

  alpha = log(h/100) + a*t/(b+t);
  dew_point = (b*alpha)/(a-alpha);

  // Display to OLED 
  display.clearDisplay();
  display.setCursor(0,0);

  t = t * 9/5 + 32;
  dew_point = dew_point * 9/5 +32;

  // The SCD4x sensor needs 5 seconds between readings. Sampling rate of 0.2Hz.
  if(count > 4) {
    error = scd.getDataReadyStatus(dataReady);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute getDataReadyStatus(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    while (!dataReady) {
        delay(100);
        error = scd.getDataReadyStatus(dataReady);
        if (error != NO_ERROR) {
            Serial.print("Error trying to execute getDataReadyStatus(): ");
            errorToString(error, errorMessage, sizeof errorMessage);
            Serial.println(errorMessage);
            return;
        }
    }
    //
    // If ambient pressure compenstation during measurement
    // is required, you should call the respective functions here.
    // Check out the header file for the function definition.
    error =
        scd.readMeasurement(scd4_co2Concentration, scd4_temperature, scd4_relativeHumidity);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }

    count = 0;
  }


  display.print(t);
  display.print(" *F   ");

  display.print(h);
  display.println(" %RH");

  display.print("DWPNT: ");
  display.print(dew_point);
  display.println(" *F");

  display.println();
  display.print("VOC Index: ");
  display.println(voc_index);

  // display.print("Pressure = ");
  display.print(pressure / 100.0);
  display.println(" hPa");

  display.print("CO2 [ppm]: ");
  display.println(scd4_co2Concentration);

  display.print(measuredBatteryVoltage);
  display.print(" Volts   ");

  timestamp = millis() - timestamp;
  
  display.print(timestamp);
  display.print(" ms");

  delay(10);
  yield();
  display.display(); // actually display all of the above

  delay(446);  // Loop time needs to be 1000 ms. 480 = 1000 - 520
  
  // Serial.print("Read duration (ms): ");
  Serial.println(timestamp);
  // Serial.println();

  count++;
}
