#include <Wire.h>
#include <VL53L0X.h>
#include <FastLED.h>

#define NUM_LEDS 150
#define LED_PIN 2

CRGB leds [NUM_LEDS];
VL53L0X sensor;

const int buzzer = 9; // buzzer pin digital 9
const int tickPot = A0; // tickPot pin analog 0
const int chunkPot = A1; // chunkPot pin analog 1
const int widthPot = A2; // widthPot pin analog 2

unsigned long buzzerDuration = 400; //update rate of toggling buzzer
unsigned long tickPotChangeRate = 200; // update rate of tickPot
unsigned long flagUpdate = 300; // update rate of flag (chunkPot and widthPot)

unsigned long tick = 100; // tick interval speed limit of around 5ms, this line is used for the initial tick rate that is changed almost instantly

unsigned long startTimeLight = 0;
unsigned long startTimeBuzzerOn = 0;
unsigned long potLastChanged = 0;
unsigned long flagLastUpdated = 0;


unsigned long speed1 = 120; // cant make this adaptable with calculation due to speed limit
unsigned long speed2 = 40;
unsigned long speed3 = 30; // Passing 20 ms is not recommended unless width is < 3.
unsigned long speed4 = 20;
unsigned long speed5 = 10; // maximum speed of the LED strip is around 5 ms, any faster will not change the result

int greenPos = 0; // do not set green pos to > NUM_LEDS - 1

int chunkAmount = 3; //this line is used for the initial chunks that is changed almost instantly
int chunkReserve = chunkAmount; // used to verify if chunk amount has changed

int width = 1; //this line is used for the initial width that is changed almost instantly
int widthReserve = chunkAmount; // used to verify if width amount has changed

int minimumSpace = 1; // used in the widthCheck() at high widthPot readings to specify the desired off leds between each jump

int start = 20; // can move green up to NUM_LEDS - 1, changes where the lights begin at lightconfig()
bool flag = true; // flag for checking if the chunkPot and widthPot require lightconfig() to activate.


void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(10);
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
  Wire.begin();

  //sensor connection/quality check
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {} // if sensor fails, the code will stay in this loop
  } 
  sensor.setMeasurementTimingBudget(20000);
  // 20ms = fastest setting of sensor, cannot bypass tick of 20 without the line slowing down on jumps.
}


void loop() {
  FastLED.setCorrection (TypicalPixelString);
  unsigned currentTime = millis();

  if (flag) {
    lightconfig(chunkAmount, width);
    flag = false;
  }

  if (currentTime - startTimeLight >= tick) {
    startTimeLight = currentTime;
    
    // Shift the LED colors each tick
    CRGB last = leds[NUM_LEDS - 1];
    leds[0] = last; // this forces the green LED to maintain the same position
    for(int line = NUM_LEDS - 1 ; line > 0 ; line--) {
      leds[line] = leds[line - 1];
    }
    leds[0] = last;
    leds[greenPos] = CRGB::Green; // can move green up to NUM_LEDS - 1
    FastLED.show();
  }

    //buzzer activity
    buzzerCheck(currentTime, startTimeBuzzerOn);
    //speed pot activity
    tickPotCheck(currentTime, potLastChanged);
    //update Flag
    flagUpdater(currentTime);
}

void lightconfig(int chunkAmount, int width) {
  fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
  switch(chunkAmount) {
    case 1:
      nt1(width, start);
      break;
    case 2:
      nt2(width, start);
      break;
    case 3:
      nt3(width, start);
      break;
    case 4:
      nt4(width, start);
      break;
    case 6:
      nt6(width, start);
      break;
    default:
      break;
  } 
}


void buzzerCheck(unsigned long currentTime, unsigned long potLastChanged) {
  if (leds[greenPos + 1] == CRGB::Red) {
    if (sensor.readRangeSingleMillimeters() < 700) { //sensor reading limits the speed of the arduino
      tone(buzzer, 300);
      startTimeBuzzerOn = currentTime;
    }
    }

    if (currentTime - startTimeBuzzerOn >= buzzerDuration) {
      noTone(buzzer);
    }
}

void tickPotCheck(unsigned long currentTime, unsigned long potLastChanged) {
    if (currentTime - potLastChanged >= tickPotChangeRate) {
    potLastChanged = currentTime;
    int speedPotValue = analogRead(tickPot);
    //Serial.println(tick);
      if (speedPotValue < 100) {
        tick = speed1;
      } else if (speedPotValue < 350) {
        tick = speed2;
      } else if (speedPotValue < 600) {
        tick = speed3;
      } else if (speedPotValue < 850) {
        tick = speed4;
      } else {
        tick = speed5;
      }
  }
}

void flagUpdater (unsigned long currentTime) {
  if (currentTime - flagLastUpdated >= flagUpdate) {
    flagLastUpdated = currentTime;
    chunkCheck();
    widthCheck();
    printData();
  }
}

void chunkCheck () {
  int chunkPotValue = analogRead(chunkPot);
    Serial.println(chunkPotValue);
    if (chunkPotValue < 100) {
      chunkAmount = 1;
    } else if (chunkPotValue < 350) {
      chunkAmount = 2;
    } else if (chunkPotValue < 600) {
      chunkAmount = 3;
    } else if (chunkPotValue < 850) {
      chunkAmount = 4;
    } else {
      chunkAmount = 6;
    }
    if (chunkAmount != chunkReserve) {
      chunkReserve = chunkAmount;
      flag = true;
      Serial.println("chunkAmount changed " + chunkAmount);
    }
}

void widthCheck () {
  int widthPotValue = analogRead(widthPot);
  Serial.println(widthPotValue);
  if (widthPotValue < 100) {
    width = 1;
  } else if (widthPotValue < 250) {
    width = 2;
  } else if (widthPotValue < 400) {
    width = 4;
  } else if (widthPotValue < 550) {
    width = 8;
  } else if (widthPotValue < 700) {
    width = 12;
  } else if (widthPotValue < 850) {
    width = 16;
  } else {
    width = ((NUM_LEDS / chunkAmount) - minimumSpace);
  }
  if (width != widthReserve) {
    widthReserve = width;
    flag = true;
    Serial.println("width changed " + width);
  }
}

void printData() {
  int jpm = chunkAmount * (60000 / (NUM_LEDS * tick));
  int airtime = 100 * ((width * chunkAmount) / NUM_LEDS);
  Serial.println("Current Data:");
  Serial.println("Tick Rate: " + tick + " Chunks: " + chunkAmount + " Width: " + width);
  Serial.println("Jumps Per Min: " + jpm + " Airtime: " + airtime + "%");
}

void nt1(int width, int start) {
  leds[start] = CRGB:: Black; // 1/1
  for (int j = 1 ; j <= width; j++) {
    leds[(start+j) % NUM_LEDS] = CRGB::Red;
  } 
}

void nt2(int width, int start) {
  nt1(width, start);  // 2/2
  leds[(start + (NUM_LEDS / 2)) % NUM_LEDS] = CRGB::Black; // 1/2
  for(int j = 1 ; j <= width; j++) {
    leds[(start + (NUM_LEDS / 2) + j) % NUM_LEDS] = CRGB::Red; 
  }
}

void nt3(int width, int start) {
  nt1(width, start); // 3/3
  leds[(start + (NUM_LEDS / 3)) % NUM_LEDS] = CRGB::Black;  // 1/3
  leds[(start + (2 * NUM_LEDS / 3)) % NUM_LEDS] = CRGB::Black;  // 2/3
  for(int j = 1 ; j <= width; j++) {
    leds[(start + (NUM_LEDS / 3) + j) % NUM_LEDS] = CRGB::Red;
    leds[(start + (2 * NUM_LEDS / 3) + j) % NUM_LEDS] = CRGB::Red;
  }
}

void nt4(int width, int start) {
  nt2(width, start); // 4/4 2/4
  leds[(start + (NUM_LEDS / 4)) % NUM_LEDS] = CRGB::Black;  // 1/4
  leds[(start + (3 * NUM_LEDS / 4)) % NUM_LEDS] = CRGB::Black; // 3/4
  for(int j = 1 ; j <= width; j++) {
    leds[(start + (NUM_LEDS / 4) + j) % NUM_LEDS] = CRGB::Red;
    leds[(start + (3 * NUM_LEDS / 4) + j) % NUM_LEDS] = CRGB::Red;
  }    
}

void nt6(int width, int start) {
  nt3(width, start); // 6/6 2/6 4/6
  nt2(width, start); // 6/6 3/6
  leds[(start + (NUM_LEDS / 6)) % NUM_LEDS] = CRGB::Black;  // 1/6
  leds[(start + (5 * NUM_LEDS / 6)) % NUM_LEDS] = CRGB::Black; // 5/6  
  for(int j = 1 ; j <= width; j++) {
    leds[(start + (NUM_LEDS / 6) + j) % NUM_LEDS] = CRGB::Red;
    leds[(start + (5 * NUM_LEDS / 6) + j) % NUM_LEDS] = CRGB::Red;
  }
}

