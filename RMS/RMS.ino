//RFID https://hanixdiy.blogspot.com/2019/11/huzzah32-rfid-base-module.html

String validCardUID = "211 147 150 26";
bool safeLocked = true;


//Pin Definitions
#define SS_PIN  21  // ES32 Feather
#define RST_PIN 17 // esp32 Feather. Could be others.
#define redLED 27
#define greenLED 33




//RFID
#include <MFRC522.h>
MFRC522 rfid(SS_PIN, RST_PIN);




#include "sensitiveInformation.h"


#define FORMAT_SPIFFS_IF_FAILED true

// Wifi & Webserver
#include "WiFi.h"
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>



// Temp Sensor
#include <Wire.h>
#include "Adafruit_ADT7410.h"

// TFT Screen
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include "Adafruit_miniTFTWing.h"

#include <SPI.h>

#define TFT_CS         14
#define TFT_DC         32
#define TFT_RST        -1
Adafruit_miniTFTWing ss;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// Date and time functions using a PCF8523 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Create the ADT7410 temperature sensor object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410();

AsyncWebServer server(80);


//Motor Setup

#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *myMotor = AFMS.getMotor(4);
// You can also make another motor on port M2
//Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(2);


boolean LEDOn = false; // State of Built-in LED true=on, false=off.
#define LOOPDELAY 100

// ESP32Servo Start
#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
int servoPin = 12;
int blindHight = 0;
// ESP32Servo End




void setup() {
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  Serial.println("Start of setup");
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }
  delay(1000);


  // ESP32Servo Start
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
  // ESP32Servo End
  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
    // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    // Follow instructions in README and install
    // https://github.com/me-no-dev/arduino-esp32fs-plugin
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  if (!ss.begin()) {
    Serial.println("seesaw init error!");
    while (1);
  }
  else Serial.println("seesaw started");


  tft.setRotation(0);

  ss.tftReset();
  ss.setBacklight(0x0); //set the backlight fully on
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("TFT set");


  Wifi Configuration
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println();
  Serial.print("Connected to the Internet");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  routesConfiguration(); // Reads routes from routesManagement

  server.begin();
  rfid.PCD_Init(); // init MFRC522
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  // The PCF8523 can be calibrated for:
  //        - Aging adjustment
  //        - Temperature compensation
  //        - Accuracy tuning
  // The offset mode to use, once every two hours or once every minute.
  // The offset Offset value from -64 to +63. See the Application Note for calculation of offset values.
  // https://www.nxp.com/docs/en/application-note/AN11247.pdf
  // The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
  // and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
  // Note: any previous calibration should cancelled prior to any new observation period.
  // Example - RTC gaining 43 seconds in 1 week
  float drift = 43; // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  float period_sec = (7 * 86400);  // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  float deviation_ppm = (drift / period_sec * 1000000); //  deviation in parts per million (μs)
  float drift_unit = 4.34; // use with offset mode PCF8523_TwoHours
  // float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  int offset = round(deviation_ppm / drift_unit);
  // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
  // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration

  if (!tempsensor.begin()) {
    Serial.println("Couldn't find ADT7410!");
    while (1);
  }
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  safeController();
  safeUnlocker();
  builtinLED();
  windowBlinds();
  tftDrawText(readTempature(), ST77XX_WHITE, 0, 0, 3);
  fanController(29.00);
  delay(LOOPDELAY); // To allow time to publish new code.
}



String readTempature() {
  /*
     Read the Tempature off of the sensor
     and returns it as a string
     @param: NULL
     @return: String
  */
  float c = tempsensor.readTempC();
  String cString = String(c);
  return (cString);
}

void fanController(float tempatureThreshold) {
  float currentTemp = tempsensor.readTempC();
  if (currentTemp > tempatureThreshold) {
    myMotor->setSpeed(150);
    myMotor->run(FORWARD);
  }
  else {
    myMotor->run(RELEASE);
  }


}


void tftDrawText(String text, uint16_t color, int x, int y, int textSize) {
  /*
   * Draw given text on screen
   * at given x,y and at given size
   * @pram Text String x int (X position of text) y (Y position of text) int textSize int
   * @return: void
   */
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(x, y);
  tft.setTextSize(textSize);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void builtinLED() {
  /*
   * Used for turn on and off the led
   * when the user request so 
   * through the website
   * 
   * @pram: void
   * @return: void
   */
  if (LEDOn) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}


void windowBlinds() {
  /*
   * When the user uses the joystick on the screen
   * open the window blids (Servo) by 10
   * if the windows are at their max limit 
   * stop the user from being able to change the blinds hight
   * @pram void
   * @return: void
   * 
   */
  uint32_t buttons = ss.readButtons();
  if (! (buttons & TFTWING_BUTTON_UP)) {
    myservo.write(blindHight);
    if (blindHight == 180) {

    }
    else {
      blindHight += 10;
    }
  }

  else if (! (buttons & TFTWING_BUTTON_DOWN)) {
    myservo.write(blindHight);
    if (blindHight == 0) {

    }
    else {
      blindHight -= 10;
    }
  }
}


String readRFID() {
  /*
   * Read the RFID UID of the RFID chip on the reader
   * @param: Void
   * @return: String
   */
  String uidOfCardRead = "";

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("RFID/NFC Tag Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // print UID in Serial Monitor in the hex format
  Serial.print("UID:");
  for (int i = 0; i < rfid.uid.size; i++) {
    uidOfCardRead += rfid.uid.uidByte[i] < 0x10 ? " 0" : " ";
    uidOfCardRead += rfid.uid.uidByte[i];
  }
  Serial.println();

  rfid.PICC_HaltA(); // halt PICC
  rfid.PCD_StopCrypto1(); // stop encryption on PCD

  uidOfCardRead.trim();
  return (uidOfCardRead);
}

void safeController() {
  /*
   * Checks if the RFID on the reader matches the trusted UID
   * to unlock the safe
   * @param: Void
   * @return: void
   */
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      if (readRFID() == validCardUID) {
        safeLocked = !safeLocked;
        Serial.println(safeLocked);
      }
      else {
        Serial.println("worng card");
      }

    }
  }
}

void safeUnlocker() {
  /*
   * Checks if the safeLocked var is ture or false
   * if ture lock the safe
   * if false unlock the safe
   * @param: Void
   * @return: void
   */
  if (safeLocked) {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
  }
  else {
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  }
}

void logEvent(String dataToLog) {
  /*
     Log entries to a file stored in SPIFFS partition on the ESP32.
  */
  // Get the updated/current time
  DateTime rightNow = rtc.now();
  char csvReadableDate[25];
  sprintf(csvReadableDate, "%02d,%02d,%02d,%02d,%02d,%02d,",  rightNow.year(), rightNow.month(), rightNow.day(), rightNow.hour(), rightNow.minute(), rightNow.second());

  String logTemp = csvReadableDate + dataToLog + "\n"; // Add the data to log onto the end of the date/time

  const char * logEntry = logTemp.c_str(); //convert the logtemp to a char * variable

  //Add the log entry to the end of logevents.csv
  appendFile(SPIFFS, "/logEvents.csv", logEntry);

  // Output the logEvents - FOR DEBUG ONLY. Comment out to avoid spamming the serial monitor.
  //  readFile(SPIFFS, "/logEvents.csv");

  Serial.print("\nEvent Logged: ");
  Serial.println(logEntry);
}
