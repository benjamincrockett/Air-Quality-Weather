/*************************************************** 
  SHT4x Humidity & Temp Sensor:       https://www.adafruit.com/product/5665
  Adafruit 128x64 OLED FeatherWing:   https://www.adafruit.com/product/4650
  Adafruit Feather M4 Express:        https://www.adafruit.com/product/3857
  Adafruit SGP40 Air Quality Sensor:  https://www.adafruit.com/product/4829
  STEMMA QT / Qwiic JST SH 4-Pin Cable - 50mm Long  https://www.adafruit.com/product/4399
 
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
 ****************************************************/

#include <Adafruit_SHT4x.h>
#include "Adafruit_SGP40.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <math.h>

#define VBATPIN A6

float measuredBatteryVoltage = 0.0;
float t, h = 0.0;
double heat_index = 0.0;
double dew_point = 0.0;
double alpha = 0.0;
double a = 17.625;
double b = 243.04;

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_SGP40 sgp = Adafruit_SGP40();

void setup() {
  Serial.begin(115200);
  // while (!Serial) { delay(10); } // Wait for serial console to open!

  Serial.println("SHT4x, SGP40, and 128x64 OLED Weather Station");

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

  display.print(t);
  display.print(" *F   ");

  display.print(h);
  display.println(" %RH");

  //display.println();
  display.print("Alpha: ");
  display.println(alpha);

  display.print("Dewpoint: ");
  display.println(dew_point);

  display.println();
  display.print("VOC Index: ");
  display.println(voc_index);

  display.print("Battery: ");
  display.print(measuredBatteryVoltage);
  display.println(" Volts");

  timestamp = millis() - timestamp;

  display.print("Loop Time: ");
  display.print(timestamp);

  delay(10);
  yield();
  display.display(); // actually display all of the above

  delay(446);  // Loop time needs to be 1000 ms. 445 = 1000 - 555
  
  // Serial.print("Read duration (ms): ");
  Serial.println(timestamp);
  // Serial.println();
}
