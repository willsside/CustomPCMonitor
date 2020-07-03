#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TimeLib.h>

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define PURPLE  0x881F
#define PINK    0xF81A

// ===================== TEMP SETTINGS ===================== //
// All Temps in C
int CPUtemp = 40;         // Initial CPU temp
int GPUtemp = 40;         // Initial CPU temp
int MAX_TEMP = 80;        // Max displayed Temp (bars cut off at this max)
int MIN_TEMP = 20;        // Min displayed Temp (bars cut off at this min)
// ========================================================= //

// =================== COLOR SETTINGS ====================== //
#define BACKGROUND  BLACK    // Background Color
#define secColor1 WHITE      // 1st Second hand color
#define secColor2 BACKGROUND // 2nd Second hand color
#define centralCircleColor MAGENTA // Color of permanent circle
#define timeColor WHITE      // Color of time (including AM/PM and colon)
#define dateColor MAGENTA    // Color of date (day and date)
#define compColor WHITE      // Color of text "CPU" and "GPU"
#define tempColor WHITE      // Color of numerical text
#define CPUbarColor CYAN     // Color of CPU temp bar
#define GPUbarColor PURPLE     // Color of GPU temp bar
// ======================================================== //

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

//Custom defines
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message
#define TEMP_HEADER  'C'   // Header tag for temp sync message

// Time parameters
time_t prevTime = now();
unsigned long start;
unsigned long oneSec = 1000;
unsigned long pctime;
const unsigned long DEFAULT_TIME = 1592611200; // June 20 2020
bool skip = false;
String daysOfWeek[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

//Indicies for each clock digit
int textSize = 8;
int hrs1 = 6 * (textSize) * 0;
int hrs2 = 6 * (textSize) * 1;
int min1 = 6 * (textSize) * 3;
int min2 = 6 * (textSize) * 4;
int char_w = 6 * textSize - 1;
int char_h = 8 * textSize - 1;

//Init Hands
double secHand;
double CPUhand;
double GPUhand;
int curSecDeg;
int curCPUDeg;
int curGPUDeg;
int curSecColor = secColor1;

// Circle Parameters
#define circRad 120
double inc = 0.5;
double deg2rad = 0.0174533;

bool tempChanged = true;

void setup(void) {
  // LCD Setup
  Serial.begin(9600);
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(0);

  //Sync time with PC
  tft.setTextColor(RED);
  tft.setTextSize(3);
  tft.setCursor(0, 0);
  tft.println("Waiting for  Time Sync...");
  //  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for time sync message... ");
  while (timeStatus() != timeSet) {
    if (Serial.available()) {
      processSyncMessage(true);
    }
  }
  Serial.println("Time synced!");
  Serial.println("Initializing Clock...");

  init0();

  //Clear input buffer
  clearInputBuff();
}

void loop(void) {
  start = millis();





  //Check for time / temp sync
  if (Serial.available()) {
    processSyncMessage();
  }

  //Calc new sec hand
  curSecDeg = second(now()) * 6;
  if (curSecDeg < 6) {
    curSecDeg = 360;

    //Update Time
    updateTime();

  } else if (curSecDeg < 10) {
    //Switch color
    if (curSecColor == secColor1) {
      curSecColor = secColor2;
    } else if (curSecColor == secColor2) {
      curSecColor = secColor1;
    }
  }

  //Update Temperatures
  if (tempChanged) {
    updateTemps();
  }

  //Calc temp hands
  if (GPUtemp < MIN_TEMP) {curGPUDeg = 0;} else if (GPUtemp > MAX_TEMP) {curGPUDeg = 180;}
  else {curGPUDeg = float(GPUtemp - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) * 180;}
  if (CPUtemp < MIN_TEMP) {curCPUDeg = 0;} else if (CPUtemp > MAX_TEMP) {curCPUDeg = 180;}
  else {curCPUDeg = float(CPUtemp - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) * 180;}

  //Update hands
  secHand = drawCircleTrig_CW(tft.width() / 2, tft.height() - circRad, circRad, 5, secHand, curSecDeg, curSecColor);
  CPUhand = drawCircleTrig_CW(tft.width() / 2, tft.height() - circRad, circRad - 20, 15, CPUhand, curCPUDeg , CPUbarColor);
  GPUhand = drawCircleTrig_CCW(tft.width() / 2, tft.height() - circRad, circRad - 20, 15, GPUhand, curGPUDeg , GPUbarColor);




  start = millis() - start;
  if (start > oneSec) {
    Serial.println(start);
    Serial.println("Loop process takes longer than 1 second, skipping delay");
    skip = true;
  }
  if (skip) {
    skip = false;
  } else {
    delay(oneSec - start);
  }
  Serial.println(start);
}


// ====================================== INIT FUNCTIONS ======================================= //

// ============================================================================================= //
//Initialize First state of clock
void init0() {
  //Fill screen black and add purple circle
  tft.fillScreen(BACKGROUND);
  drawCircleTrig_CW(tft.width() / 2, tft.height() - circRad, circRad - 5, 15, 0, 360, centralCircleColor);

  //Init Temps
  curCPUDeg = float(CPUtemp - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) * 180;
  curGPUDeg = float(GPUtemp - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) * 180;
  CPUhand = drawCircleTrig_CW(tft.width() / 2, tft.height() - circRad, circRad - 20, 15, 0, curCPUDeg, CPUbarColor);
  GPUhand = drawCircleTrig_CCW(tft.width() / 2, tft.height() - circRad, circRad - 20, 15, 0, curGPUDeg, GPUbarColor);

  //Init Clock
  initTime();

  //Init Temp names
  tft.setTextColor(compColor);
  tft.setTextSize(3);
  tft.setCursor(tft.width() / 2 - 72, tft.height() - circRad - 23);
  tft.println("CPU  GPU");
}

void initTime() {
  //Clear previous time
  tft.fillRect(0, 0, char_w * 2, char_h, BACKGROUND);
  tft.fillRect(min1, 0, char_w * 2, char_h, BACKGROUND);

  //Get Current Time
  time_t curTime = now();

  //Init second hand
  secHand = drawCircleTrig_CW(tft.width() / 2, tft.height() - circRad, circRad, 5, 0, second(curTime) * 6, curSecColor);

  //Create time char
  String timeString;
  if (hourFormat12(curTime) == 0) {
    timeString += "12";
  } else {
    timeString += padDigit(hourFormat12(curTime), true);
  }
  timeString += " ";
  timeString += padDigit(minute(curTime), false);

  //Print Time
  tft.setTextColor(timeColor);
  tft.setTextSize(textSize);
  tft.setCursor(0, -10);
  tft.print("  :");
  tft.setCursor(0, 0);
  tft.print(timeString);

  //Init AM/PM
  changeM(tft.width() / 2 - 15, 40);

  //Init Date
  updateDate(curTime);
}

// ===================================== HELPER FUNCTIONS ====================================== //

// ============================================================================================= //
double drawCircleTrig_CW(int x0, int y0, int r, int thick, double degStart, double degEnd, uint16_t color) {
  if (degStart > degEnd) {
    drawCircleTrig_CW(x0, y0, r, thick, degEnd, degStart, BACKGROUND);
    return degEnd;
  }
  for (double deg = 90 + degStart; deg < 90 + degEnd; deg += inc) {
    for (int cr = r; cr > r - thick; cr--) {
      tft.drawPixel(x0 + cr * cos(deg * deg2rad), y0 + cr * sin(deg * deg2rad), color);
    }
  }
  if (degEnd == 360) {
    degEnd = 0;
  }
  return degEnd;
}

// ============================================================================================= //
double drawCircleTrig_CCW(int x0, int y0, int r, int thick, double degStart, double degEnd, uint16_t color) {
  if (degStart > degEnd) {
    drawCircleTrig_CCW(x0, y0, r, thick, degEnd, degStart, BACKGROUND);
    return degEnd;
  }
  for (double deg = 90 - degStart; deg > 90 - degEnd; deg -= inc) {
    for (int cr = r; cr > r - thick; cr--) {
      tft.drawPixel(x0 + cr * cos(deg * deg2rad), y0 + cr * sin(deg * deg2rad), color);
    }
  }
  if (degEnd == 360) {
    degEnd = 0;
  }
  return degEnd;
}

// ============================================================================================= //
void updateDate(time_t curTime) {
  tft.fillRect(0, 70, 36, 15, BACKGROUND);
  tft.fillRect(tft.width() - 66, 70, 60, 15, BACKGROUND);
  tft.setTextColor(dateColor);
  tft.setTextSize(2);
  tft.setCursor(0, 70);
  tft.print(daysOfWeek[weekday(curTime) - 1] + " ");
  tft.setCursor(tft.width() - 66, 70);
  tft.print(padDigit(month(curTime), true) + "-" + padDigit(day(curTime), false));

}

// ============================================================================================= //
//Change to AM or PM
void changeM(int x, int y) {
  // draw over current AM / PM
  tft.fillRect(x, y, 22, 18, BACKGROUND);

  // display AM or PM
  tft.setCursor(x, y);
  tft.setTextColor(timeColor);
  tft.setTextSize(2);
  if (isAM()) {
    tft.print("AM");
  } else {
    tft.print("PM");
  }
}

// ============================================================================================= //
void updateTime() {

  tft.setTextColor(timeColor);
  tft.setTextSize(textSize);
  tft.setCursor(0, 0);

  // Get current time
  time_t curTime = now();

  //Check if minutes need to be changed
  if (second(curTime) % 60 == 0 && minute(curTime) % 10 == 0) {
    // Fill both minute digits
    tft.fillRect(min1, 0, char_w * 2, char_h, BACKGROUND);

    //Write minute digits
    tft.setCursor(min1, 0);
    tft.print(String(minute(curTime)));

    //Check if hours need to be changed
    if (minute(curTime) % 60 == 0 && hourFormat12(curTime) % 10 == 0) {
      //Finish Minutes
      tft.setCursor(min2, 0);
      tft.print("0");

      // Fill both hour digits
      tft.fillRect(hrs1, 0, char_w * 2, char_h, BACKGROUND);

      //Write hour digits
      tft.setCursor(hrs1, 0);
      tft.print(String(hourFormat12(curTime)));

    } else if (minute(curTime) % 60 == 0) {
      //Finish Minutes
      tft.setCursor(min2, 0);
      tft.print("0");

      // Fill only a single hour digit
      tft.fillRect(hrs2, 0, char_w, char_h, BACKGROUND);

      // If it's 1 PM or 1 AM, erase the 1 from the 12 before
      if (hour(curTime) == 1 || hour(curTime) == 13) {
        tft.fillRect(hrs1, 0, char_w, char_h, BACKGROUND);
      }

      //Write hour digit
      tft.setCursor(hrs2, 0);
      tft.print(String(hourFormat12(curTime) % 10));
    }
    changeM(tft.width() / 2 - 15, 40);
    updateDate(curTime);
  } else if (second(curTime) % 60 == 0) {
    // Fill only a single minute digit
    tft.fillRect(min2, 0, char_w, char_h, BACKGROUND);

    //Write minute digit
    tft.setCursor(min2, 0);
    tft.print(String(minute(curTime) % 10));
  }
}

// ============================================================================================= //
void updateTemps() {
  tft.fillRect(tft.width() / 2 - 56, tft.height() - circRad, 38, 23, BACKGROUND);
  tft.fillRect(tft.width() / 2 + 17, tft.height() - circRad, 38, 23, BACKGROUND);
  tft.setTextColor(tempColor);
  tft.setTextSize(3);
//  tft.setCursor(tft.width() / 2 - 72, tft.height() - circRad);
//  tft.println(" " + String(CPUtemp) + "  " + String(GPUtemp));
  tft.setCursor(tft.width() / 2 - 56, tft.height() - circRad);
  tft.println(String(CPUtemp));
  tft.setCursor(tft.width() / 2 + 17, tft.height() - circRad);
  tft.println(String(GPUtemp));
  tempChanged = false;
}

// ============================================================================================= //
void changeTemps(int in)
{
  Serial.println("Input Int: " + String(in));
  GPUtemp = String(in).substring(2, 4).toInt();
  CPUtemp = String(in).substring(0, 2).toInt();
  tempChanged = true;
}

// ============================================================================================= //
String padDigit(int digit, bool first)
{
  if (digit >= 10) {
    return String(digit);
  } else {
    if (first) {
      return " " + String(digit);
    } else {
      return "0" + String(digit);
    }
  }
}

// ============================================================================================= //
void processSyncMessage(bool setupTime) {
  char c = Serial.read();
  if (c == TIME_HEADER) {
    pctime = Serial.parseInt();
    if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than June 20 2020)
      setTime(pctime); // Sync Arduino clock to the time received on the serial port
    } else {
      Serial.println("INPUT TIME IS OLDER THAN June 20, 2020");
      return;
    }
    if (!setupTime) {
      initTime();
      //Clear input buffer
      clearInputBuff();
    }
    skip = true;
  } else if (c == TEMP_HEADER) {
    changeTemps(parseIntCus());
    clearInputBuff();
//    requestSync();
//    Serial.println("PAST");
  }
}
void processSyncMessage() {
  processSyncMessage(false);
}

// ============================================================================================= //
int parseIntCus() {
  byte ndx = 0;
  byte numChars = 64;
  char receivedChars[numChars];
  char endMarker = '\n';
  char rc;
  bool newData = false;
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }

  return String(receivedChars).substring(0, 4).toInt();
}

//// ============================================================================================= //
//time_t requestSync()
//{
//  Serial.write(TIME_REQUEST);
//  return 0; // the time will be sent later in response to serial mesg
//}

// ============================================================================================= //
void clearInputBuff() {
  while (Serial.available()) {
    char c = Serial.read();
  }
}
