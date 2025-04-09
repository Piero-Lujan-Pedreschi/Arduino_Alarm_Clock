#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>
#include "pitches.h"

DS3231 clock;
RTCDateTime dt;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int buttonPinL = 4;
int buttonPinR = 5;
const int buzzerPin = 3;

int alarmHour = -1;
int alarmMinute = -1;

int melody[] = {
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4,
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4,
  NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_E4, NOTE_F4, NOTE_G4
};

int noteDurations[] = {
  4, 4, 4, 4,
  4, 4, 4, 4,
  4, 4, 2,
  4, 4, 2
};

void setup() {
  pinMode(buttonPinL, INPUT_PULLUP);
  pinMode(buttonPinR, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Alarm clock activated");
  clock.begin();
  dt = clock.getDateTime();

  lcd.begin(12, 2);
  displayDate();
}

bool alarmTriggered = false;
static unsigned long lastTimeUpdate = 0;

void loop() {
  dt = clock.getDateTime();
  displayTime();

  int currentHour = dt.hour;
  int currentMinute = dt.minute;
  if (currentHour == alarmHour && currentMinute == alarmMinute && !alarmTriggered) {
    Serial.println("Alarm had been triggered");
    triggerAlarm();
  }
  if (alarmTriggered) {
    playAlarm();
  }
  
  handleButtons();
  updateTempMessage();

  delay(200);
}

void promptAlarm() {
  lcd.setCursor(0, 1); lcd.print("                "); lcd.setCursor(0, 1);
  lcd.print("Set alarm time");

  alarmHour = getAlarmHour();
  alarmMinute = getAlarmMinute();
  String alarmTime = String(alarmHour) + ":" + String(alarmMinute);

  Serial.println("Alarm set at: ");
  Serial.println(String(alarmHour) + ":" + String(alarmMinute));

  if (alarmHour > -1 && alarmMinute > -1) {
    displayTempMessage("Alarm at: " + alarmTime);
  }
}

void triggerAlarm() {
  alarmTriggered = true;
  lcd.clear();
  lcd.print("Time to wake up!");
  displayTime();
}

void playAlarm1() {
  while (alarmTriggered) {
    for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(int) && alarmTriggered; thisNote++) {
      int duration = 1000 / noteDurations[thisNote];
      tone(buzzerPin, melody[thisNote], duration);

      unsigned long noteStart = millis();
      while (millis() - noteStart < duration * 1.3) {
        // 🔁 Check button during the tone
        if (digitalRead(buttonPinR) == LOW || digitalRead(buttonPinL) == LOW) {
          noTone(buzzerPin); // Stop immediately
          return; // Exit the song
        }
      }

      noTone(buzzerPin); // Stop between notes
    }

    // Short pause between melody loops
    unsigned long pauseStart = millis();
    while (millis() - pauseStart < 1000) {
      if (digitalRead(buttonPinR) == LOW || digitalRead(buttonPinL) == LOW) {
        noTone(buzzerPin);
        return;
      }
    }
  }

  noTone(buzzerPin);
}

unsigned long alarmStartTime = 0;       // When the alarm started
const unsigned long alarmDuration = 8UL * 60UL *1000UL; // Alarm runs for 10 seconds

// Buzzer control
bool isBuzzerOn = false;                // Is the buzzer currently on?
unsigned long lastBeepTime = 0;         // Last time buzzer beeped
const unsigned long beepInterval = 500; // Time between beeps (e.g., 500ms)
const unsigned long beepDuration = 100; // Length of each beep (e.g., 100ms)
const int beepTone = 1000; 

void playAlarm() {
  unsigned long currentMillis = millis();

  // Start alarm timer once
  if (alarmStartTime == 0) {
    alarmStartTime = currentMillis;
  }

  // Stop after timeout
  if (currentMillis - alarmStartTime >= alarmDuration) {
    Serial.println("Alarm stopped after timeout");
    noTone(buzzerPin);
    isBuzzerOn = false;
    alarmTriggered = false;
    alarmStartTime = 0;
    return;
  }

  // Stop if button is pressed
  if (digitalRead(buttonPinL) == LOW || digitalRead(buttonPinR) == LOW) {
    Serial.println("Alarm stopped by button");
    noTone(buzzerPin);
    isBuzzerOn = false;
    alarmStartTime = 0;
    return;
  }

  // Beep logic
  if (!isBuzzerOn && currentMillis - lastBeepTime >= beepInterval) {
    tone(buzzerPin, beepTone);
    isBuzzerOn = true;
    lastBeepTime = currentMillis;
  }

  if (isBuzzerOn && currentMillis - lastBeepTime >= beepDuration) {
    noTone(buzzerPin);
    isBuzzerOn = false;
  }
}



void resetDisplay() {
  lcd.clear();
  displayDate();
  displayTime();
  Serial.println("display reset");
}

void displayDate() {
  dt = clock.getDateTime();
  lcd.print(dt.month);      // Print month
  lcd.print("/");           // Print slash
  lcd.print(dt.day);        // Print day
  lcd.print("/");           // Print slash
  lcd.print(dt.year);
}

void displayTime() {
  dt = clock.getDateTime();
  lcd.setCursor(0, 1);       // Set cursor to the second row
  lcd.print("                ");  // Clear the row
  lcd.setCursor(0, 1);
  lcd.print(dt.hour); lcd.print(":");
  lcd.print(dt.minute); lcd.print(":");
  lcd.print(dt.second);
}

int getAlarmHour() {
  Serial.println("Set alarm hour: ");

  while (!Serial.available()) {
    if (digitalRead(buttonPinL) == LOW) {
      Serial.println("Alarm minute input cancelled.");
      return alarmHour;
    }
  }
  String alarmHour = Serial.readStringUntil("\n");
  return alarmHour.toInt();
}

int getAlarmMinute() {
  Serial.println("Set alarm minute: ");

  while (!Serial.available()) {
    if (digitalRead(buttonPinL) == LOW) {
      Serial.println("Alarm minute input cancelled.");
      return alarmMinute;
    }
  }
  String alarmMinute = Serial.readStringUntil("\n");
  return alarmMinute.toInt();
  
}

void handleButtons() {
  if (digitalRead(buttonPinL) == LOW && !alarmTriggered && alarmHour != -1 && alarmMinute != -1) { //cancel any alarms that are already set
    Serial.println("Alarm has been cancelled");
    alarmHour = -1;
    alarmMinute = -1;
    displayTempMessage("Alarm cleared");
    return;
  } else if (digitalRead(buttonPinR) == LOW && !alarmTriggered) {   //set alarm from clock in display start with right button
    Serial.println("Set alarm button has been pressed");
    promptAlarm();
    return;
  } else if (digitalRead(buttonPinR) == LOW && alarmTriggered) {   //stop alarm and return to time display with right button 
    Serial.println("Alarm had been disabled");
    alarmTriggered = false;
    alarmHour = -1;
    alarmMinute = -1;
    resetDisplay();
  } else if (digitalRead(buttonPinL) == LOW && alarmTriggered) {  //snooze the alarm to go off in 8 minutes with the left button
    Serial.println("SNOOZE! Alarm will go if in 8 minutes");
    alarmTriggered = false;
    alarmMinute+=1;
    if (alarmMinute >= 60) {
      alarmHour++;
      alarmMinute -= 60;
    }
    if (alarmHour >= 24) {
      alarmHour -= 24;
    }
    displayTempMessage("SNOOZE");
    return;
  }
}

unsigned long messageStartTime = 0;
bool isMessageActive = false;
String tempMessage = "";

void displayTempMessage(String message) {
  if (!isMessageActive) {
    lcd.clear();
    lcd.print(message);
    Serial.println("temp msg");
    tempMessage = message;
    messageStartTime = millis();
    isMessageActive = true;
  }
}

void updateTempMessage() {
  if (isMessageActive && millis() - messageStartTime >= 5000) {
    lcd.clear();
    isMessageActive = false;
    tempMessage = "";

    resetDisplay();
  }
}
