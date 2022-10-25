
/**************************************************************************/
/*!
This is a demo for the Adafruit ADT7410 breakout
----> http://www.adafruit.com/products/4089
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
*/
/**************************************************************************/

#include <Wire.h>
#include "Adafruit_ADT7410.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include "Adafruit_miniTFTWing.h"

#include <SPI.h>

#define TFT_CS         14                                            
#define TFT_DC         32
#define TFT_RST        -1



// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410();
Adafruit_miniTFTWing ss;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);


void setup() {
  Serial.begin(9600);

  Serial.println("ADT7410 demo");
    if (!ss.begin()) {
    Serial.println("seesaw init error!");
    while(1);
  }
  else Serial.println("seesaw started");
    

  tft.setRotation(3);

  ss.tftReset();
  ss.setBacklight(0x0); //set the backlight fully on
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  tft.fillScreen(ST77XX_BLACK);
  // Make sure the sensor is found, you can also pass in a different i2c
  // address with tempsensor.begin(0x49) for example
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find ADT7410!");
    while (1);
  }

  // sensor takes 250 ms to get first readings
  delay(250);
}

void loop() {
  // Read and print out the temperature, then convert to *F
  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  Serial.print("Temp: "); Serial.print(c); Serial.print("*C\t"); 
  Serial.print(f); Serial.println("*F");
  String cString = String(c);
  tftDrawText(cString,ST77XX_WHITE);
  delay(1000);
}

void tftDrawText(String text, uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}
