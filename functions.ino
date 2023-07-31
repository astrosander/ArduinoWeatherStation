void(* resetFunc) (void) = 0; 

void checkBrightness() {
  if (analogRead(PHOTO) < BRIGHT_THRESHOLD) {   // если темно
    analogWrite(BACKLIGHT, LCD_BRIGHT_MIN);
    CurBright = LCD_BRIGHT_MIN;
#if (LED_MODE == 0)
    LED_ON = (LED_BRIGHT_MIN);
#else
    LED_ON = (255 - LED_BRIGHT_MIN);
#endif
  } else {       
    CurBright = LCD_BRIGHT_MAX;
    analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);
#if (LED_MODE == 0)
    LED_ON = (LED_BRIGHT_MAX);
#else
    LED_ON = (255 - LED_BRIGHT_MAX);
#endif
  }
  if (dispCO2 < 800) setLED(2);
  else if (dispCO2 < 1200) setLED(3);
  else if (dispCO2 >= 1200) setLED(1);
}

void Settings(){
  setRGB(164, 0, 252);
    IsMode = 1;
    lcd.clear();
    String menu[4] = {"Restarting", "Stopwatch", "Brightness", "Exit"};
    
    for(int i = 0; i <= 3; i++)
    {
      lcd.setCursor(1, i);   
      lcd.print(menu[i]); 
    }
    lcd.setCursor(0, CurMode);
    lcd.write(62);    
}

void Setting(int mod){
  lcd.clear();
  
  
  switch (mod) {
    case 0:
      setRGB(252, 0, 101);
      delay(100);
      resetFunc();
      
    case 1: 
       setRGB(255, 0, 72);
       lcd.setCursor(0, 0);
       lcd.print("Press BUTTON");

       elapsed = 0;
       InMode = true;
       IsPause = true;
       
       break;
      
    case 2:
      InMode = true;
      break;
      
    case 3:
      IsMode = false; 
      mode = 0;;
      loadClock();
      drawClock(hrs, mins, 0, 0, 1);
      if (DISPLAY_TYPE == 1) drawData();
      drawSensors();
      break; 
  }
}

void modesTick() {
  button.tick();
  
  if(IsMode)
  {
    if(InMode){
        switch (CurMode) {
          case 1:
            if(button.isClick()){
              if(IsPause) startTime = millis() - elapsed;
              else setRGB(255, 0, 72);
              IsPause = !IsPause;
            }
            
            if(button.isHolded()){
              InMode = false;
              Settings();
              }
            break;
            
          case 2:
              if(button.isClick()){
                BrightStep = (BrightStep+1)%7;
                CurBright = BrightNess[BrightStep]; 
                analogWrite(BACKLIGHT, CurBright);
                LED_ON = (CurBright);
                BrightAccess = false;
              }
              if(button.isDouble())BrightAccess = true;
            if(button.isHolded()){
              InMode = false;
              Settings();
              }
            break;
        }
      return;
    }
    
    if(button.isClick()){
      lcd.setCursor(0, CurMode);
      lcd.print(" ");
      CurMode = (CurMode+1)%4;
      lcd.setCursor(0, CurMode);
      Serial.println(CurMode);
      lcd.write(62); 
    }

    if (button.isHolded()){
      Setting(CurMode);
    }
    
    return;
  }
  
  if (button.isDouble()) {
    Settings();
    return;
  }
  
  boolean changeFlag = false;
  if (button.isClick()) {
    mode++;

#if (CO2_SENSOR == 1)
    if (mode > 8) mode = 0;
#else
    if (mode > 6) mode = 0;
#endif
    changeFlag = true;
  }
  if (button.isHolded()) {
    mode = 0;
    changeFlag = true;
  }
  
  if (changeFlag) {
    if (mode == 0) {
      lcd.clear();
      loadClock();
      drawClock(hrs, mins, 0, 0, 1);
      if (DISPLAY_TYPE == 1) drawData();
      drawSensors();
    } else {
      lcd.clear();
      loadPlot();
      redrawPlot();
    }
  }
}

void redrawPlot() {
  lcd.clear();

  switch (mode) {
    case 1: drawPlot(0, 3, 15, 4, TEMP_MIN, TEMP_MAX, (int*)tempHour, "t hr");
      break;
    case 2: drawPlot(0, 3, 15, 4, TEMP_MIN, TEMP_MAX, (int*)tempDay, "t day");
      break;
    case 3: drawPlot(0, 3, 15, 4, HUM_MIN, HUM_MAX, (int*)humHour, "h hr");
      break;
    case 4: drawPlot(0, 3, 15, 4, HUM_MIN, HUM_MAX, (int*)humDay, "h day");
      break;
    case 5: drawPlot(0, 3, 15, 4, PRESS_MIN, PRESS_MAX, (int*)pressHour, "p hr");
      break;
    case 6: drawPlot(0, 3, 15, 4, PRESS_MIN, PRESS_MAX, (int*)pressDay, "p day");
      break;
    case 7: drawPlot(0, 3, 15, 4, CO2_MIN, CO2_MAX, (int*)co2Hour, "c hr");
      break;
    case 8: drawPlot(0, 3, 15, 4, CO2_MIN, CO2_MAX, (int*)co2Day, "c day");
      break;
  }
}

void readSensors() {
  
  dispTemp = dht.readTemperature();//bme.readTemperature();
  dispHum = dht.readHumidity();//bme.readHumidity();
  dispPres = analogRead(PHOTO);//(float)bme.readPressure() * 0.00750062;
  dispCO2 = mhz19.getPPM();

//  TempMn = min(TempMn, dispTemp);
//  TempMx = max(TempMx, dispTemp);
//  HumMn = min(HumMn, dispHum);
//  HumMx = max(HumMx, dispHum);
//  CO2MN = min(CO2MN, dispCO2);
//  CO2MX = max(CO2MX, dispCO2);  
  
  Serial.println(String(dispTemp) + "°C   "  + String(dispHum) + "%   light: " + String(dispPres) + "   " + String(dispCO2) + " ppm");

  
  
  if (dispCO2 < 800) setLED(2);
  else if (dispCO2 < 1200) setLED(3);
  else if (dispCO2 >= 1200) setLED(1);
}




void drawSensors() {
#if (DISPLAY_TYPE == 1)
  // дисплей 2004
  lcd.setCursor(0, 2);
  lcd.print(String(dispTemp, 1));
  lcd.write(223);
  lcd.setCursor(6, 2);
  lcd.print(" " + String(dispHum) + "%  ");

#if (CO2_SENSOR == 1)
  lcd.print(String(dispCO2) + " ppm");

  if (dispCO2 < 1000) lcd.print(" ");
#endif

  lcd.setCursor(0, 3);
  lcd.print(String(dispPres));
  lcd.print(F("       "));
  
  lcd.setCursor(16, 3);
  int dayofweek = now.dayOfTheWeek();
  lcd.print(dayNames[dayofweek]);
  
  //lcd.print(String(dispRain) + "%");

#else
  // дисплей 1602
  lcd.setCursor(0, 0);
  lcd.print(String(dispTemp, 1));
  lcd.write(223);
  lcd.setCursor(6, 0);
  lcd.print(String(dispHum) + "% ");

#if (CO2_SENSOR == 1)
  lcd.print(String(dispCO2) + "ppm");
  if (dispCO2 < 1000) lcd.print(" ");
#endif

  lcd.setCursor(0, 1);
  lcd.print(String(dispPres) + " mm  rain ");
  lcd.print(String(dispRain) + "% ");
#endif
}

void plotSensorsTick() {
  // 4 минутный таймер
  if (hourPlotTimer.isReady()) {
    for (byte i = 0; i < 14; i++) {
      tempHour[i] = tempHour[i + 1];
      humHour[i] = humHour[i + 1];
      pressHour[i] = pressHour[i + 1];
      co2Hour[i] = co2Hour[i + 1];
    }
    tempHour[14] = dispTemp;
    humHour[14] = dispHum;
    co2Hour[14] = dispCO2;

    if (PRESSURE) pressHour[14] = dispRain;
    else pressHour[14] = dispPres;
  }

  // 1.5 часовой таймер
  if (dayPlotTimer.isReady()) {
    long averTemp = 0, averHum = 0, averPress = 0, averCO2 = 0;

    for (byte i = 0; i < 15; i++) {
      averTemp += tempHour[i];
      averHum += humHour[i];
      averPress += pressHour[i];
      averCO2 += co2Hour[i];
    }
    averTemp /= 15;
    averHum /= 15;
    averPress /= 15;
    averCO2 /= 15;

    for (byte i = 0; i < 14; i++) {
      tempDay[i] = tempDay[i + 1];
      humDay[i] = humDay[i + 1];
      pressDay[i] = pressDay[i + 1];
      co2Day[i] = co2Day[i + 1];
    }
    tempDay[14] = averTemp;
    humDay[14] = averHum;
    pressDay[14] = averPress;
    co2Day[14] = averCO2;
  }


}

boolean dotFlag;
void clockTick() {
  dotFlag = !dotFlag;
  if (dotFlag) {          // каждую секунду пересчёт времени
    secs++;
    if (secs > 59) {      // каждую минуту
      secs = 0;
      mins++;
      if (mins <= 59 && mode == 0) drawClock(hrs, mins, 0, 0, 1);
    }
    if (mins > 59) {      // каждый час
      now = rtc.now();
      secs = now.second();
      mins = now.minute();
      hrs = now.hour();
      if (mode == 0) drawClock(hrs, mins, 0, 0, 1);
      if (hrs > 23) {
        hrs = 0;
      }
      if (mode == 0 && DISPLAY_TYPE) drawData();
    }
    if (mode == 0 && !IsMode) {//(DISP_MODE == 2 && mode == 0 && !IsMode) {
      lcd.setCursor(16, 1);
      if (secs < 10) lcd.print(" ");
      lcd.print(secs);
    }
  }
  if(IsMode) return;
  if (mode == 0) drawdots(7, 0, dotFlag);
  if (dispCO2 >= 1200) {
    if (dotFlag) setLED(1);
    else setLED(0);
  }
}
