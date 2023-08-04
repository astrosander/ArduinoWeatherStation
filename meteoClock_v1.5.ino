byte BrightNess[7] = {0, 1, 51, 102, 153, 204, 255};

byte CurMode = 0;
byte BrightStep = 5;
bool IsMode = false;
bool InMode = false;
bool IsPause = false;
byte CurBright = 255;
bool BrightAccess = true;

// ------------------------- SETTINGS --------------------
#define RESET_CLOCK 1 // reset clock for the time of firmware loading (for module with non-removable battery). Don't forget to set 0 and flash again!
#define SENS_TIME 5000 // update time of sensor readings on the screen, milliseconds
#define LED_MODE 0 // RGB LED type: 0 - main cathode, 1 - main anode

// brightness control
#define BRIGHT_CONTROL 1 // 0/1 - disable/enable brightness control (when disabled, brightness will always be max).
#define BRIGHT_THRESHOLD 15 // signal value below which the brightness will switch to minimum (0-1023)
#define LED_BRIGHT_MAX 255 // max. brightness of CO2 LED (0 - 255)
#define LED_BRIGHT_MIN 1 // min CO2 LED brightness (0 - 255)
#define LCD_BRIGHT_MAX 255 // max brightness of the display backlight (0 - 255)
#define LCD_BRIGHT_MIN 1 // min brightness of the display backlight (0 - 255)

#define BLUE_YELLOW 1 // yellow colour instead of blue (1 yes, 0 no), but due to peculiarities of connection yellow is not so bright.
#define DISP_MODE 2 // display in the upper right corner: 0 - year, 1 - day of week, 2 - seconds
#define WEEK_LANG 0 // weekday language: 0 - English, 1 - Russian (transliteration)
#define DEBUG 0 // display the log of sensor initialisation at startup. It doesn't work for the 1602 display! But it is duplicated through the port!
#define PRESSURE 0 // 0 - pressure graph, 1 - rain forecast graph (instead of pressure). Don't forget to correct the limits of the groafic
#define CO2_SENSOR 1 // enable or disable support/output from CO2 sensor (1 on, 0 off)
#define DISPLAY_TYPE 1 // display type: 1 - 2004 (large), 0 - 1602 (small)
#define DISPLAY_ADDR 0x27 // display board address: 0x27 or 0x3f. If the display does not work - change the address! The address is not specified on the display itself

// display limits for graphs
#define TEMP_MIN 15
#define TEMP_MAX 35
#define HUM_MIN 0
#define HUM_MAX 100
#define PRESS_MIN 0
#define PRESS_MAX 1024
#define CO2_MIN 300
#define CO2_MAX 2000

// pins
#define BACKLIGHT 10
#define PHOTO A3

#define MHZ_RX 2
#define MHZ_TX 3

#define LED_COM 7
#define LED_R 9
#define LED_G 6
#define LED_B 5
#define BTN_PIN 4

#define BL_PIN 10 // display backlight pin
#define PHOTO_PIN 0 // photoresistor pin


// libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

DHT dht(11, DHT11);

#if (DISPLAY_TYPE == 1)
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);
#else
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 16, 2);
#endif

#include "RTClib.h"
RTC_DS3231 rtc;
DateTime now;


#if (CO2_SENSOR == 1)
#include <MHZ19_uart.h>
MHZ19_uart mhz19;
#endif

#include <GyverTimer.h>
GTimer_ms sensorsTimer(SENS_TIME);
GTimer_ms drawSensorsTimer(SENS_TIME);
GTimer_ms clockTimer(500);
GTimer_ms hourPlotTimer((long)20 * 1000); // 4 minutes
GTimer_ms dayPlotTimer((long)4 * 60 * 1000); // 1.6 hours
GTimer_ms brightTimer(2000);
GTimer_ms StopWatch(10);

#include "GyverButton.h"
GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);

int8_t hrs, mins, secs;
byte mode = 0;
/*
  0 clock and data
  1 temperature graph per hour
  2 temperature graph for a day
  3 humidity graph per hour
  4 humidity graph per day
  5 pressure graph per hour
  6 pressure graph per day
  7 carbon dioxide graph per hour
  8 carbon dioxide graph per day
*/

// variables for output
float dispTemp;
byte dispHum;
int dispPres;
int dispCO2;
int dispRain;

// arrays of graphs
int tempHour[15], tempDay[15];
int humHour[15], humDay[15];
int pressHour[15], pressDay[15];
int co2Hour[15], co2Day[15];
int delta;
uint32_t pressure_array[6];
uint32_t sumX, sumY, sumX2, sumXY;
float a, b;
byte time_array[6];

// symbols
// chart
byte row8[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};

// numbers
uint8_t LT[8] = {0b00111,  0b01111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
uint8_t UB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
uint8_t RT[8] = {0b11100,  0b11110,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
uint8_t LL[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b01111,  0b00111};
uint8_t LB[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};
uint8_t LR[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11110,  0b11100};
uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};
uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};

void drawDig(byte dig, byte x, byte y) {
  switch (dig) {
    case 0:
      lcd.setCursor(x, y); // set cursor to column 0, line 0 (first row)
      lcd.write(0);  // call each segment to create
      lcd.write(1);  // top half of the number
      lcd.write(2);
      lcd.setCursor(x, y + 1); // set cursor to colum 0, line 1 (second row)
      lcd.write(3);  // call each segment to create
      lcd.write(4);  // bottom half of the number
      lcd.write(5);
      break;
    case 1:
      lcd.setCursor(x + 1, y);
      lcd.write(1);
      lcd.write(2);
      lcd.setCursor(x + 2, y + 1);
      lcd.write(5);
      break;
    case 2:
      lcd.setCursor(x, y);
      lcd.write(6);
      lcd.write(6);
      lcd.write(2);
      lcd.setCursor(x, y + 1);
      lcd.write(3);
      lcd.write(7);
      lcd.write(7);
      break;
    case 3:
      lcd.setCursor(x, y);
      lcd.write(6);
      lcd.write(6);
      lcd.write(2);
      lcd.setCursor(x, y + 1);
      lcd.write(7);
      lcd.write(7);
      lcd.write(5);
      break;
    case 4:
      lcd.setCursor(x, y);
      lcd.write(3);
      lcd.write(4);
      lcd.write(2);
      lcd.setCursor(x + 2, y + 1);
      lcd.write(5);
      break;
    case 5:
      lcd.setCursor(x, y);
      lcd.write(0);
      lcd.write(6);
      lcd.write(6);
      lcd.setCursor(x, y + 1);
      lcd.write(7);
      lcd.write(7);
      lcd.write(5);
      break;
    case 6:
      lcd.setCursor(x, y);
      lcd.write(0);
      lcd.write(6);
      lcd.write(6);
      lcd.setCursor(x, y + 1);
      lcd.write(3);
      lcd.write(7);
      lcd.write(5);
      break;
    case 7:
      lcd.setCursor(x, y);
      lcd.write(1);
      lcd.write(1);
      lcd.write(2);
      lcd.setCursor(x + 1, y + 1);
      lcd.write(0);
      break;
    case 8:
      lcd.setCursor(x, y);
      lcd.write(0);
      lcd.write(6);
      lcd.write(2);
      lcd.setCursor(x, y + 1);
      lcd.write(3);
      lcd.write(7);
      lcd.write(5);
      break;
    case 9:
      lcd.setCursor(x, y);
      lcd.write(0);
      lcd.write(6);
      lcd.write(2);
      lcd.setCursor(x + 1, y + 1);
      lcd.write(4);
      lcd.write(5);
      break;
    case 10:
      lcd.setCursor(x, y);
      lcd.write(32);
      lcd.write(32);
      lcd.write(32);
      lcd.setCursor(x, y + 1);
      lcd.write(32);
      lcd.write(32);
      lcd.write(32);
      break;
  }
}

void drawdots(byte x, byte y, boolean state) {
  byte code;
  if (state) code = 165;
  else code = 32;
  lcd.setCursor(x, y);
  lcd.write(code);
  lcd.setCursor(x, y + 1);
  lcd.write(code);
}

void drawClock(byte hours, byte minutes, byte x, byte y, boolean dotState) {
  // drawClock!
  lcd.setCursor(x, y);
  lcd.print(" ");
  lcd.setCursor(x, y + 1);
  lcd.print(" ");

  //if (hours > 23 || minutes > 59) return;
  if (hours / 10 == 0) drawDig(10, x, y);
  else drawDig(hours / 10, x, y);
  drawDig(hours % 10, x + 4, y);
  // there should be points here. By a separate function
  drawDig(minutes / 10, x + 8, y);
  drawDig(minutes % 10, x + 12, y);
}

static const char *dayNames[] = {
  "Sund",
  "Mond",
  "Tues",
  "Wedn",
  "Thur",
  "Frid",
  "Satu",
};

void drawData() {
  lcd.setCursor(15, 0);
  if (now.day() < 10) lcd.print(0);
  lcd.print(now.day());
  lcd.print(".");
  if (now.month() < 10) lcd.print(0);
  lcd.print(now.month());

  if (DISP_MODE == 0) {
    lcd.setCursor(16, 1);
    lcd.print(now.year());
  } else if (DISP_MODE == 1) {
    lcd.setCursor(16, 1);
    int dayofweek = now.dayOfTheWeek();
    lcd.print(dayNames[dayofweek]);
  }
}

void drawPlot(byte pos, byte row, byte width, byte height, float min_val, float max_val, int *plot_array, String label) {
  int max_value = -32000;
  int min_value = 32000;
  for (byte i = 0; i < 15; i++) {
    if (plot_array[i] > max_value) max_value = plot_array[i];
    if (plot_array[i] && plot_array[i] < min_value) min_value = plot_array[i];
  }

  min_val = min_value*0.95;
  max_val = max_value*1.05;
  
  lcd.setCursor(16, 0); lcd.print(max_value);
  lcd.setCursor(16, 1); lcd.print(label);
  lcd.setCursor(16, 2); lcd.print(plot_array[14]);
  lcd.setCursor(16, 3); lcd.print(min_value);

  for (byte i = 0; i < width; i++) { // each column of parameters
    int fill_val = plot_array[i];
    fill_val = constrain(fill_val, min_val, max_val);
    byte infill, fract;
    // find the number of integer blocks considering the minimum and maximum to be displayed on the chart
    if (plot_array[i] > min_val)
      infill = floor((float)(plot_array[i] - min_val) / (max_val - min_val) * height * 10);
    else infill = 0;
    fract = (float)(infill % 10) * 8 / 10; // find the number of remaining strips
    infill = infill / 10;

    for (byte n = 0; n < height; n++) { // for all lines of the chart
      if (n < infill && infill > 0) { // while we are below the level
        lcd.setCursor(i, (row - n)); // fill with full cells
        lcd.write(0);
      }
      if (n >= infill) { // if we have reached the level
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract); // fill fractional cells
        else lcd.write(16); // if fractional == 0, fill the empty one
        for (byte k = n + 1; k < height; k++) { // fill everything on top with empty ones
          lcd.setCursor(i, (row - k));
          lcd.write(16);
        }
        break;
      }
    }
  }
}

void loadClock() {
  lcd.createChar(0, LT);
  lcd.createChar(1, UB);
  lcd.createChar(2, RT);
  lcd.createChar(3, LL);
  lcd.createChar(4, LB);
  lcd.createChar(5, LR);
  lcd.createChar(6, UMB);
  lcd.createChar(7, LMB);
}

void loadPlot() {
  lcd.createChar(0, row8);
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}

#if (LED_MODE == 0)
byte LED_ON = (LED_BRIGHT_MAX);
byte LED_OFF = (LED_BRIGHT_MIN);
#else
byte LED_ON = (255 - LED_BRIGHT_MAX);
byte LED_OFF = (255 - LED_BRIGHT_MIN);
#endif

void setRGB(byte r, byte g, byte b){ 
  if (!LED_MODE) {
    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
  } else {
    analogWrite(LED_R, 255);
    analogWrite(LED_G, 255);
    analogWrite(LED_B, 255);
  }  
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void setLED(byte color) {
  switch (color) {    // 0 off, 1 red, 2 green, 3 blue (or yellow)
    case 0:
      break;
    case 1: setRGB(LED_ON, 0, 0);
      break;
    case 2: setRGB(0, LED_ON, 0);
      break;
    case 3:
      if (!BLUE_YELLOW) setRGB(0, 0, LED_ON);
      else setRGB(LED_ON - 50, LED_ON, 0);
      break;
  }
}

void setup() {
  
  Serial.begin(9600);
  
  lcd.clear();
  loadClock();
  drawClock(hrs, mins, 0, 0, 1);
  if (DISPLAY_TYPE == 1) drawData();
  drawSensors();(BACKLIGHT, OUTPUT);

  pinMode(LED_COM, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLED(0);

  digitalWrite(LED_COM, LED_MODE);
  analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  dht.begin();
  

#if (DEBUG == 1 && DISPLAY_TYPE == 1)
  boolean status = true;

  setLED(1);

#if (CO2_SENSOR == 1)
  lcd.setCursor(0, 0);
  lcd.print(F("MHZ-19... "));
  Serial.print(F("MHZ-19... "));
  mhz19.begin(MHZ_TX, MHZ_RX);
  mhz19.setAutoCalibration(false);
  mhz19.getStatus(); // first request, returns -1 in any case
  delay(500);
  if (mhz19.getStatus() == 0) {
    lcd.print(F("OK"));
    Serial.println(F("OK"));
  } else {
    lcd.print(F("ERROR"));
    Serial.println(F("ERROR"));
    status = false;
  }
#endif

  setLED(2);
  lcd.setCursor(0, 1);
  lcd.print(F("RTC... "));
  Serial.print(F("RTC... "));
  delay(50);
  if (rtc.begin()) {
    lcd.print(F("OK"));
    Serial.println(F("OK"));
  } else {
    lcd.print(F("ERROR"));
    Serial.println(F("ERROR"));
    status = false;
  }

  setLED(3);
  lcd.setCursor(0, 2);
  lcd.print(F("BME280... "));
  Serial.print(F("BME280... "));
  delay(50);

  setLED(0);
  lcd.setCursor(0, 3);
  if (status) {
    lcd.print(F("All good"));
    Serial.println(F("All good"));
  } else {
    lcd.print(F("Check wires!"));
    Serial.println(F("Check wires!"));
  }
  while (1) {
    lcd.setCursor(14, 1);
    lcd.print("P: ");
    lcd.setCursor(16, 1);
    lcd.print(analogRead(PHOTO), 1);
    Serial.println(analogRead(PHOTO));
    delay(300);
  }
#else

#if (CO2_SENSOR == 1)
  mhz19.begin(MHZ_TX, MHZ_RX);
  mhz19.setAutoCalibration(false);
#endif
  rtc.begin();
  //bme.begin(&Wire);
#endif
  if (RESET_CLOCK || rtc.lostPower()){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  now = rtc.now();
  secs = now.second();
  mins = now.minute();
  hrs = now.hour();

  //bme.takeForcedMeasurement();
  uint32_t Pressure = 10;//bme.readPressure();
  for (byte i = 0; i < 6; i++) { // counter from 0 to 5
    pressure_array[i] = Pressure; // fill the whole array with current pressure
    time_array[i] = i; // fill the time array with numbers 0 - 5
  }
  
  if (DISPLAY_TYPE == 1) {
    loadClock();
    drawClock(hrs, mins, 0, 0, 1);
    drawData();
  }
  readSensors();
  drawSensors();

  setRGB(255, 0, 0);
  delay(500);
  setRGB(0, 255, 0);
  delay(500);
  setRGB(0, 0, 255);
  delay(500);
}

unsigned long startTime = millis();
unsigned long elapsed = 0;

void loop() {
  modesTick();
  if (clockTimer.isReady()) clockTick();
  
  if(InMode && CurMode == 1 && !IsPause && StopWatch.isReady()){
      
      setRGB(0, 255, 0);
      elapsed = millis() - startTime;
      
      int milliseconds = elapsed % 1000;
      int seconds = (elapsed / 1000) % 60;
      int minutes = (elapsed / 60000) % 60;
      int hours = elapsed / 3600000;
      
      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      lcd.print(hours);
      lcd.print(":");
      if (minutes < 10) lcd.print("0");
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) lcd.print("0");
      lcd.print(seconds);
      lcd.print(".");
      if (milliseconds < 100) lcd.print("0");
      if (milliseconds < 10) lcd.print("0");
      lcd.print(milliseconds);
      delay(10);
      
      return;
  }
  if(InMode && CurMode == 2 && StopWatch.isReady()){
    setRGB(85, 255, 0);
    lcd.setCursor(0, 0);
    lcd.print("Bright: ");
    lcd.setCursor(8, 0);
    lcd.print(" ");
    lcd.setCursor(8, 0);
    lcd.print(String(CurBright)); 
    lcd.setCursor(0, 1);
    lcd.print("BrightAccess: ");
    lcd.print(BrightAccess);
  }
  if(IsMode) return;
  
  if (brightTimer.isReady() && BrightAccess) checkBrightness(); // brightness
  if (sensorsTimer.isReady()) readSensors();

#if (DISPLAY_TYPE == 1)
  plotSensorsTick(); // there are several timers inside here for recalculating graphs (per hour, per day and forecast)
  //modesTick(); // here we catch button presses and switch modes
  if (mode == 0) { // in the "main screen" mode
    if (drawSensorsTimer.isReady()) drawSensors(); // update sensor readings on the display with SENS_TIME period
  } else { // in any of the charts
    if (hourPlotTimer.isReady()) redrawPlot(); // redraw the chart
  }
#else
  if (drawSensorsTimer.isReady()) drawSensors();
#endif
}
