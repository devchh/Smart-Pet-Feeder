#include <Wire.h>
#include <Servo.h>
#include "millisDelay.h"
#include "rgb_lcd.h"
#include "Ultrasonic.h"
#include "DHT.h"

// operational constants
const int log_delay = 60000;
const int dispense_time = 900;
const int button_off_time = 2000;
const int update_dht_time = 3000;
const int led_flash_time = 500;
const int dispense_delay = 30000; // 30 seconds for demonstration
const int buzzer_intensity = 2;
const int light_cutoff = 75;



// pins
const int ledPin = 3;         // for displaying is empty LED pin, D3
const int buttonPin = 4;      // for feeding and turning off, D4
const int dhtPin = 8;         // for checking temp and humi
const int servoPin = 6;       // for dispensing
const int ultrasonicPin = 7;  // for checking for pet
const int buzzerPin = 5;      // 

const int lcdPin = 12;      // for displaying

// button results
const int BUTTON_NONE = 0;
const int BUTTON_DOWN = 1;
const int BUTTON_UP = 2;
const int BUTTON_HELD = 3;

// component handles
rgb_lcd lcd;                          // lcd panel handle
DHT dht = DHT(dhtPin, DHT11);         // temp humi/sensor handle
Servo servo;                          // servo handle
Ultrasonic ultrasonic(ultrasonicPin); // ultrasonic handle

// runtime vars
millisDelay buttonDelay;    // delay before turning off device
millisDelay dhtDelay;       // delay before updating temp humi
millisDelay buzzerDelay;    // the delay of buzzer
millisDelay dispenseDelay;  // the delay before feeding is allowed
millisDelay logDelay;       // the delay before variables are sent to the cloud 
bool isButtonPressed = false; // the state of the button
bool buzzerState = false;     // the state of the buzzer

// sensor variables
int light;
int humidity;
int temperature;
int dispenses;

int buttonPress() {
  int buttonState = digitalRead(buttonPin);
  //Serial.println(buttonState);

  // button pressed
  if(buttonState == LOW) {
    
    // if first time button pressed
    if (!isButtonPressed) { 
      isButtonPressed = true;
      buttonDelay.start(button_off_time);

      // button down delay timer started
      return BUTTON_DOWN; 
    }

    // if button being held
    else if (buttonDelay.justFinished()) { 
      isButtonPressed = false;

      // button is being held and timer is up
      return BUTTON_HELD; 
    }
    else {

      // currently being held but timer is not up
      return BUTTON_NONE; 
    }

    
  // button not currently pressed, but button 
  } else if (isButtonPressed) {
      isButtonPressed = false;
      buttonDelay.finish();

      // button held but timer not up
      return BUTTON_UP; 
  } else {

    // button not pressed and not held not pressed last check
    return BUTTON_NONE;
  }
}

// if feeder empty
bool feederEmpty() {
  // light sensor at bottom of feeder bright
  int light = analogRead(A3);
  //Serial.println(light);
  return light > light_cutoff;
}

void updateBuzzer(bool empty) {
  if (empty) {
    if (buzzerDelay.justFinished()) {
      if (buzzerState == true) 
      {
        analogWrite(buzzerPin, 0);
        buzzerDelay.start(800);
      }
      else {
        analogWrite(buzzerPin, buzzer_intensity);  
        buzzerDelay.start(200);
      }
      buzzerState = !buzzerState;
    }
  }
  else {
    analogWrite(buzzerPin, 0);
    buzzerState = false; 
  }
}

void updateScreen() {
  if (dhtDelay.justFinished()) {
    displayTempHum();
    dhtDelay.start(update_dht_time);
  }
}

void updateLog() {
  if (logDelay.justFinished()) {
    char[] outputString 
    Serial.println("{ ")
  }
}

bool petDetected() {
  // get distance in front of feeder
  int dist = ultrasonic.MeasureInCentimeters();
  //Serial.println(dist);
  // if less than 20cm the sensor returns 0
  return dist == 0;
}

bool dispenseAllowed() {
  // if wait has finished, more food is allowed
  return dispenseDelay.justFinished();
}

// dispenses food
void dispense() {
  // clear screen
  lcd.setCursor(0,0);
  lcd.print("    Feeding     ");
  lcd.setCursor(0,1);
  lcd.print("                ");

  // spin servo
  servo.write(0);

  // progressively add loading bar
  lcd.setCursor(0,1);  
  for (int i = 0; i < 16; i++)
  {
    lcd.print("#");
    delay(dispense_time / 16);
  }

  // spin servo back
  servo.write(180);
  
  // setup lcd display
  displayTempHum();
  dhtDelay.start(update_dht_time);
  
  // start delay
  dispenseDelay.start(dispense_delay);
}

// display on lcd time remaining in seconds
void displayWaitTime() {
  int time_left = dispenseDelay.remaining() / 1000;
  lcd.setCursor(0,0);
  lcd.print("       ");
  lcd.print(time_left);
  lcd.print("       ");
  lcd.setCursor(0,1);
  lcd.print("                ");
  delay(500);
  
  Serial.println("reached");
}

// display the temperature and humidity
void displayTempHum() {
  // get temperature and humidity
  auto temp = dht.readTemperature();
  auto humi = dht.readHumidity();

  // print temperature line 1
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C     ");

  // print humidity line 2
  lcd.setCursor(0,1);
  lcd.print("Humi: ");
  lcd.print(humi);
  lcd.print("%     ");
}

void turnOff() {
  buzzerDelay.finish();
  exit(0);
}

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // initiate handles
  lcd.begin(16, 2);
  servo.attach(servoPin);
  servo.write(180);

  // start up variables
  analogWrite(buzzerPin, 20);
  
  // set all start delays to 1 so time has elapsed when next check
  dhtDelay.start(1);    
  buzzerDelay.start(1); 
  buzzerDelay.start(1);   
  dispenseDelay.start(1); 
  log_delay.start(1);
  
  // display starting screen
  displayTempHum();
}

void loop() {
  updateScreen();

  bool empty = feederEmpty();
  
  // if empty buzz
  updateBuzzer(empty);
  
  // get button press
  switch (buttonPress()) {
    case BUTTON_HELD:
      turnOff();
      break;
      
    case BUTTON_UP: // button pressed but not held for delay
      dispense();
      break;
  }
  
  if (petDetected()) {
    if (!empty) {
      if (dispenseAllowed()) {
        dispense();
      } else {
        displayWaitTime();
      }
    }
  }
}
  
