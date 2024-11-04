#include <Arduino.h>
#include <TimeLib.h>
#include <Servo.h>

#define PIN_LIGHT 3
int start = 6;
int end = 21;
Servo myservo;
static boolean isLongFormat = true;

// single character message tags
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define FORMAT_HEADER 'F'   // Header tag indicating a date format message
#define FORMAT_SHORT  's'   // short month and day strings
#define FORMAT_LONG   'l'   // (lower case l) long month and day strings

#define TIME_REQUEST  7     // ASCII bell character requests a time sync message 

// Function declarations
void printDigits(int digits);
void processFormatMessage();
void processSyncMessage();
void digitalClockDisplay();
void controlLight();
void feedFish();
bool isDST(int day, int month, int dow);
void adjustForDST();

bool fishFed = true; // Flag to track if fish has been fed for the current hour
int lastFedHour = -1; // Variable to store the last hour the fish was fed


void setup() {
  myservo.attach(9);
  myservo.write(0);
  Serial.begin(9600);
  pinMode(PIN_LIGHT, OUTPUT); // Set the pin mode for PIN_LIGHT

  // Set the current time (example: 14:30:00 on 25th December 2023)
  int currentHour = 8;
  int currentMinute = 10;
  int currentSecond = 0;
  int currentDay = 3;
  int currentMonth = 11;
  int currentYear = 2024;

  setTime(currentHour, currentMinute, currentSecond, currentDay, currentMonth, currentYear);
  adjustForDST();
  Serial.println("Current time set to:");
  digitalClockDisplay();
}

void loop() {
  controlLight();
  feedFish();
  delay(1000);
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void processFormatMessage() {
   char c = Serial.read();
   if( c == FORMAT_LONG){
      isLongFormat = true;
      Serial.println(F("Setting long format"));
   }
   else if( c == FORMAT_SHORT) {
      isLongFormat = false;   
      Serial.println(F("Setting short format"));
   }
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 - paul, perhaps we define in time.h?

   pctime = Serial.parseInt();
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  if(isLongFormat)
    Serial.print(dayStr(weekday()));
  else  
   Serial.print(dayShortStr(weekday()));
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  if(isLongFormat)
     Serial.print(monthStr(month()));
  else
     Serial.print(monthShortStr(month()));
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void controlLight() {
  int currentHour = hour();
  int currentDay = day();
  int currentMonth = month();
  int currentDOW = weekday() - 1; // weekday() returns 1 for Sunday, 2 for Monday, etc.

  // Adjust current hour for DST
  if (isDST(currentDay, currentMonth, currentDOW)) {
    currentHour += 1;
  }

  int pinStatus = digitalRead(PIN_LIGHT); // Read the current status of the pin
  digitalClockDisplay();
  Serial.print("Start: ");
  Serial.println(start);
  Serial.print("End: ");
  Serial.println(end);
  Serial.print("PIN status: ");
  Serial.println(pinStatus);
  if (currentHour >= start && currentHour < end && pinStatus == LOW) {
    Serial.println("Light is on");
    digitalWrite(PIN_LIGHT, HIGH); // Turn on the light
  }
  if (currentHour >= end && pinStatus == HIGH) {
    Serial.println("Light is off");
    digitalWrite(PIN_LIGHT, LOW); // Turn off the light
  }
}

void feedFish() {
  int currentHour = hour();
  static int feedCount = 0; // Counter to track the number of feedings in one cycle
    Serial.print("Current Hour: ");
    Serial.println(currentHour);

    if ((currentHour == 7 || currentHour == 19) && (!fishFed || lastFedHour != currentHour)) {
      Serial.println("Feeding fish...");
      while (feedCount < 2)
      {
      
      for (int i = 70; i >= 0; i--) {
        myservo.write(i);
        delay(15);
      } // goes from 70 degrees to 0 degrees
      for (int i = 0; i <= 70; i++) {
        myservo.write(i);
        delay(15);
      } // goes from 0 degrees to 70 degrees
        feedCount++; // Increment the feed counter
      }

      fishFed = true; // Set the flag to true after feeding
      lastFedHour = currentHour; // Update the last fed hour
      feedCount = 0; // Reset the feed counter after two feedings
    } else if (currentHour != 7 && currentHour != 19) {
      fishFed = false; // Reset the flag if it's not feeding time
    }
  Serial.print("Current Hour: ");
  Serial.println(currentHour);

  if ((currentHour == 7 || currentHour == 19) && (!fishFed || lastFedHour != currentHour)) {
    Serial.println("Feeding fish...");
    for (int i = 70; i >= 0; i--) {
      myservo.write(i);
      delay(15);
    } // goes from 70 degrees to 0 degrees
    for (int i = 0; i <= 70; i++) {
      myservo.write(i);
      delay(15);
    } // goes from 0 degrees to 70 degrees

    fishFed = true; // Set the flag to true after feeding
    lastFedHour = currentHour; // Update the last fed hour
  } else if (currentHour != 7 && currentHour != 19) {
    fishFed = false; // Reset the flag if it's not feeding time
  }
}

// Function to determine if the current date is within DST period for Europe
bool isDST(int day, int month, int dow) {
  // DST starts on the last Sunday in March
  if (month == 3 && (day + 7 - dow) > 31) {
    return true;
  }
  // DST ends on the last Sunday in October
  if (month == 10 && (day + 7 - dow) > 31) {
    return false;
  }
  // DST is active between the last Sunday in March and the last Sunday in October
  if (month > 3 && month < 10) {
    return true;
  }
  return false;
}

// Function to adjust the time for DST
void adjustForDST() {
  int currentDay = day();
  int currentMonth = month();
  int currentDOW = weekday() - 1; // weekday() returns 1 for Sunday, 2 for Monday, etc.
  
  if (isDST(currentDay, currentMonth, currentDOW)) {
    setTime(hour() + 1, minute(), second(), day(), month(), year());
  }
}