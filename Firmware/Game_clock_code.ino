#include <WiFi.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

//------------WIFI settings-----------------
const char* WIFI_SSID = "Your_wifi_name";
const char* WIFI_PASSWORD = "Your_wifi_password";

const char* TZ_INFO = "PST8PDT,M3.2.0,M11.1.0";

//------------tft, buttons, and buzzer pins------------
//tft pins are different for arduino and esp32
#define TFT_SCLK 9 //XIAO D9
#define TFT_MOSI 10 //XIAO D10
#define TFT_RST 8 //XIAO D8
#define TFT_DC 6 //XIAO D4
#define TFT_CS 7 //XIAO D5
#define TFT_BL 21 //XIAO D6

#define BTN1 2 //XIAO D0
#define BTN2 3 //XIAO D1
#define BTN3 4 //XIAO D2
#define BTN4 5 //XIAO D3

#define BUZ 20 //XIAO D7

Adafruit_ST7789 tft = Adafruit_ST7789(
  TFT_CS, 
  TFT_DC, 
  TFT_MOSI, 
  TFT_SCLK, 
  TFT_RST
);

//----------Screen settings---------
#define s_width 284
#define s_height 76

//--------Colors---------------
#define black ST77XX_BLACK
#define white ST77XX_WHITE

//-------------Modes---------------
enum Mode {
  clock_mode,
  menu_mode,
  dino_mode,
  alarm_mode,
  ringing_mode

};

Mode currentMode = clock_mode;

//--------------Menu---------------
int menuSelection = 0;

const char* menuItems[] = {
  "Dino Game",
  "Alarm"
};

const int menuItemCount = 2;

//-----------Alarm-----------------

int alarmHour = 8;
int alarmMinute = 0;

bool alarmEnabled = true;
bool alarmRinging = false;

//------------Button State----------------

bool lastBTN1 = HIGH;
bool lastBTN2 = HIGH;
bool lastBTN3 = HIGH;
bool lastBTN4 = HIGH;

//--------------Dino Game---------------

int dinoX = 20;
int dinoY = 45;
int dinoVelocity = 0;
bool dinoJumping = false;
bool dinoGameOver = false;
int obstacleX = s_width;
int obstacleWidth = 8;
int obstacleHeight = 15;
unsigned long lastDinoUpdate = 0;
int dinoScore = 0;

//----------------WIFI----------------

void connectWifi(){
  tft.fillScreen(black);
  tft.setTextColor(white);
  tft.setTextSize(2);
  tft.setCursor(5,10);
  tft.print("Wifi...");
  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSOWRD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED){
      Serial.println("WiFi connected!");
      Serial.print("IP address: ");
      Serial.print(WiFi.localIP());
      tft.fillScreen(black);
      tft.setCursor(5,10);
      tft.print("WiFi OK");
      delay(1000);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed.");
    tft.fillScreen(black);
    tft.setCursor(5,10);
    tft.print("WiFi Failed");
    delay(1500);
  }
}

//-----------------------Get Live Time------------------
void setupTime(){
  Serial.println("Getting internet time...");
  configTzTime(
    TZ_INFO,
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
  );
  struct tm timeinfo;
  if(getLocalTime(&timeinfo, 10000)){
    Serial.printf(
      "Time: %02d:%02d:%02d\n",
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
  } else {
    Serial.println("Couldn't get the time.");
  }
}

//--------------Button Functions---------------
bool button1Pressed(){
  bool current = digitalRead(BTN1);
  if (lastBTN1 == HIGH && current == LOW) {
    lastBTN1 = current;
    return true;
  }
  lastBTN1 = current;
  return false;
}

bool button2Pressed(){
  bool current = digitalRead(BTN2);
  if (lastBTN2 == HIGH && current == LOW) {
    lastBTN2 = current;
    return true;
  }
  lastBTN2 = current;
  return false;
}

bool button3Pressed(){
  bool current = digitalRead(BTN3);
  if (lastBTN3 == HIGH && current == LOW) {
    lastBTN3 = current;
    return true;
  }
  lastBTN3 = current;
  return false;
}

bool button4Pressed(){
  bool current = digitalRead(BTN4);
  if (lastBTN4 == HIGH && current == LOW) {
    lastBTN4 = current;
    return true;
  }
  lastBTN4 = current;
  return false;
}

//-----------------Draw Clock-----------------

void drawClock(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo,10)) {
    tft.fillScreen(black);
    tft.setTextColor(white);
    tft.setTextSize(2);
    tft.setCursor(5,20);
    tft.print("NO INTERNET TIME");
    return;
  }
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;
  bool pm = false;
  int displayHour = hour;
  if(displayHour >= 12){
    pm = true;
  }
  if (displayHour == 0){
    displayHour = 12;
  } else if (displayHour > 12) {
    displayHour -= 12;
  }
  char timeString[20];
  sprintf(
    timeString,
    "%02d:%02d:%02d",
    displayHour,
    minute,
    second
  );
  tft.fillScreen(black);
  tft.setTextSize(4);
  tft.setCursor(5,5);
  tft.print(timeString);

  // AM/PM
  
  tft.setTextSize(1);
  tft.setCursor(250,10);
  if(pm){
    tft.print("PM");
  } else {
    tft.print("AM");
  }

  // Alarm Indicator
  tft.setTextSize(1);
  tft.setCursor(5,55);
  tft.print("ALARM ");
  if (alarmEnabled){
    tft.print(alarmHour);
    tft.print(":");
    if (alarmMinute < 10) {
      tft.print("0");
    }
    tft.print(alarmMinute);
  } else {
    tft.print("OFF");
  }
}

//------------------Clock Mode----------------------

void runClock(){
  static int lastSecond = -1;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo,10)){
    drawClock();
    return;
  }
  if (timeinfo.tm_sec != lastSecond) {
    lastSecond = timeinfo.tm_sec;
    drawClock();
  }
  if (button1Pressed()) {
    currentMode = menu_mode;
    delay(150);
  }
  if(
    alarmEnabled &&
    timeinfo.tm_hour == alarmHour &&
    timeinfo.tm_min == alarmMinute &&
    timeinfo.tm_sec == 0
  ) {
    alarmRinging = true;
    currentMode = ringing_mode;
    tone(BUZ, 1000);
  }
}

//---------------Menu------------------

void drawMenu(){
  tft.fillScreen(black);
  tft.setTextColor(white);
  tft.setTextSize(2);
  tft.setCursor(5,2);
  tft.print("MENU");
  for (int i = 0; i < menuItemCount; i++) {
    int y = 25 + i * 20;
    tft.setCursor(5,y);
    if(i == menuSelection) {
      tft.print("> ");
    } else {
      tft.print("  ");
    }
    tft.print(menuItems[i]);
  }
}

void runMenu() {
  static bool firstRun = true;
  if(firstRun) {
    drawMenu();
    firstRun = false;
  }
  
  if(button2Pressed()) {
    menuSelection--;
    if (menuSelection < 0 ) {
      menuSelection = menuItemCount - 1;
    }
    drawMenu();
    delay(150);
  }

  if (button3Pressed()) {
    menuSelection++;
    if (menuSelection >= menuItemCount) {
      menuSelection = 0;
    }
    drawMenu();
    delay(150);
  }

  if (button4Pressed()) {
    firstRun = true;
    if (menuSelection == 0) {
      startDino();
      currentMode = dino_mode;
    } else {
      currentMode = alarm_mode;
    }
    delay(150);
  }

  if (button1Pressed()) {
    firstRun = true;
    currentMode = clock_mode;
    delay(150);
  }
}

//--------------------Alarm Screen--------------------------

void drawAlarm() {
  tft.fillScreen(black);
  tft.setTextColor(white);
  tft.setTextSize(2);
  tft.setCursor(5,2);
  tft.print("ALARM");
  tft.setCursor(5,25);
  if(alarmHour < 10) {
    tft.print("0");
  }
  tft.print(alarmHour);
  tft.print(":");
  if (alarmMinute < 10) {
    tft.print("0");
  }
  tft.print(alarmMinute);
  tft.setTextSize(1);
  tft.setCursor(5,50);
  if (alarmEnabled) {
    tft.print("ON");
  } else {
    tft.print("OFF");
  }
  tft.setCursor(70,50);
  tft.print("B1 HOUR");
  tft.setCursor(140,50);
  tft.print("B2 MIN");
  tft.setCursor(210,50);
  tft.print("B3 ON/OFF");
  tft.setCursor(5,65);
  tft.print("B4 SAVE");
}

void runAlarmSetup() {
  static bool firstRun = true;
  if (firstRun) {
    drawAlarm();
    firstRun = false;
  }

  if (button1Pressed()) {
    alarmHour++;
    if (alarmHour >= 24) {
      alarmHour = 0;
    }
    drawAlarm();
    delay(150);
  }

  if (button2Pressed()){
    alarmMinute += 5;
    if (alarmMinute >= 60) {
      alarmMinute = 0;
    }
    drawAlarm();
    delay(150);
  }

  if (button3Pressed()) {
    alarmEnabled = !alarmEnabled;
    drawAlarm();
    delay(150);
  }

  if (button4Pressed()) {
    firstRun = true;
    currentMode = clock_mode;
    delay(150);
  }
}

//-------------------Dino Game-------------------

void startDino() {
  dinoX = 20;
  dinoY = 45;
  dinoVelocity = 0;
  dinoJumping = false;
  dinoGameOver = false;
  obstacleX = s_width;
  dinoScore = 0;
  lastDinoUpdate = millis();
}

void drawDino() {
  tft.fillScreen(black);
  tft.drawLine(
    0,
    60,
    s_width,
    60,
    white
  );
  tft.fillRect(
    dinoX,
    dinoY,
    10,
    15,
    white
  );
  tft.fillRect(
    dinoX + 7,
    dinoY - 5,
    8,
    8,
    white
  );
  tft.fillRect(
    obstacleX,
    45 - obstacleHeight + 15,
    obstacleWidth,
    obstacleHeight,
    white
  );
  
  tft.setTextSize(1);
  tft.setCursor(220,5);
  tft.print("S:");
  tft.print(dinoScore);
}

void runDino() {
  unsigned long now = millis();
  if (button1Pressed() && !dinoJumping) {
    dinoVelocity = -8;
    dinoJumping = true;
  }

  if (button4Pressed()) {
    currentMode = clock_mode;
    delay(150);
    return;
  }

  if (dinoGameOver) {
    if (button3Pressed()) {
      startDino();
    }
    return;
  }

  if (now-lastDinoUpdate >= 50) {
    lastDinoUpdate = now;
    dinoVelocity++;
    dinoY += dinoVelocity;
    if (dinoY >= 45){
      dinoY = 45;
      dinoVelocity = 0;
      dinoJumping = false;
    }
    obstacleX -= 4;
    if (obstacleX < -obstacleWidth) {
      obstacleX = s_width;
      dinoScore++;
    }

    if (
      dinoX + 10 > obstacleX &&
      dinoX < obstacleX + obstacleWidth &&
      dinoY + 15 > 45 - obstacleHeight
    ){
      dinoGameOver = true;
    }
    drawDino();
  }
  if (dinoGameOver) {
    tft.fillScreen(black);
    tft.setTextColor(white);
    tft.setTextSize(2);
    tft.setCursor(60,15);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setCursor(65,40);
    tft.print("B3 = RESTART");
    tft.setCursor(65,55);
    tft.print("B4 = EXIT");
  }
}

//-----------------Alarm Ringing---------------------

void runAlarmRinging() {
  static unsigned long lastBeep = 0;
  unsigned long now = millis();
  if (now - lastBeep >= 500){
    lastBeep = now;
    static bool beepState = false;
    beepState = !beepState;
    if (beepState) {
      tone(BUZ, 1200);
    } else {
      noTone(BUZ);
    }
  }

  tft.fillScreen(black);
  tft.setTextColor(white);
  tft.setTextSize(2);
  tft.setCursor(45,15);
  tft.print("ALARM!");
  tft.setTextSize(1);
  tft.setCursor(60,45);
  tft.print("B4 = STOP");
  if (button4Pressed()){
    noTone(BUZ);
    alarmRinging = false;
    currentMode = clock_mode;
    delay(150);
  }
}

//------------------Setup--------------------

void setup() {
  Serial.begin(115200);
  
  //buttons
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);

  //buzzer
  pinMode(BUZ, OUTPUT);
  noTone(BUZ);

  //tft backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  //tft
  tft.init(76,284);
  tft.setOffsets(82,18);
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(black);

  //wifi
  connectWifi();

  //time
  setupTime();

  //start clock
  currentMode = clock_mode;
  drawClock();
  Serial.println("Clock ready!");
}

//----------------------Loop------------------------

void loop() {
  switch (currentMode) {
    case clock_mode:
      runClock();
      break;
    case menu_mode:
      runMenu();
      break;
    case dino_mode:
      runDino();
      break;
    case alarm_mode:
     runAlarmSetup();
     break;
    case ringing_mode:
      runAlarmRinging();
      break;
  }
  delay(5);
}
