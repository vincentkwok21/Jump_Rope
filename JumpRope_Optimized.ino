#include <Wire.h>
#include <VL53L0X.h>
#include <FastLED.h>
#include "pitches.h" // for the playSong method

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

unsigned long startTimeLight = 0;
unsigned long startTimeBuzzerOn = 0;
unsigned long potLastChanged = 0;
unsigned long flagLastUpdated = 0;


unsigned long speed1 = 120; // cant make this adaptable with calculation due to speed limit
unsigned long speed2 = 40;
unsigned long speed3 = 30; // Passing 20 ms is not recommended unless width is < 3.
unsigned long speed4 = 20;
unsigned long speed5 = 10; // maximum speed of the LED strip is around 5 ms, any faster will not change the result

bool healthOn = true; // Setting healthOn to true will cause referencePos to change color in relation to remaining health and Hue variables
const int startingHealth = 20;
int remainingHealth = startingHealth;
int remainingHealthReserve = remainingHealth;
int highHue = 127; // upper hue range to resemble high health
int lowHue = 45;   // lower hue range to resemble low health
int criticalHue = 210; // hue displayed at 1 Health
int deadHue = 0;       // hue displayed at 0 Health (probably doesnt matter what you set here)
int hue[startingHealth];

int referencePos = 0; // do not set green pos to > NUM_LEDS - 1

unsigned long tick = 100; // tick interval speed limit of around 5ms, this line is used for the initial tick rate that is changed almost instantly
unsigned long tickReserve = tick; // used to verify if tick amount has changed

int chunkAmount = 3; //this line is used for the initial chunks that is changed almost instantly
int chunkReserve = chunkAmount; // used to verify if chunk amount has changed

int width = 1; //this line is used for the initial width that is changed almost instantly
int widthReserve = chunkAmount; // used to verify if width amount has changed

int minimumSpace = 10; // used in the widthCheck() at high widthPot readings to specify the desired off leds between each jump

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
  generateHueArray(startingHealth);
  Serial.println("StartingHealth:" + startingHealth);
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
    if(healthOn){
    leds[referencePos] = refHueUpdate();
    } else{
    leds[referencePos] = CRGB::Green;
    } // can move green up to NUM_LEDS - 1
    FastLED.show();
  }

    //buzzer activity
    buzzerCheck(currentTime, startTimeBuzzerOn);
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
  if (leds[referencePos + 1] == CRGB::Red) {
    if (sensor.readRangeSingleMillimeters() < 700) { //sensor reading limits the speed of the arduino
      tone(buzzer, 300);
      startTimeBuzzerOn = currentTime;
      healthReduction(healthOn);
    }
    }

    if (currentTime - startTimeBuzzerOn >= buzzerDuration) {
      noTone(buzzer);
    }
}

void flagUpdater (unsigned long currentTime) {
  if (currentTime - flagLastUpdated >= flagUpdate) {
    flagLastUpdated = currentTime;
    tickPotCheck(); // check for tick change
    chunkCheck();   // check for chunk change
    widthCheck();   // check for width change
  }
}

void tickPotCheck() {
  int speedPotValue = analogRead(tickPot);
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
    if (tick != tickReserve) {
    tickReserve = tick;
    flag = true;
    printData();
  }

}

void chunkCheck () {
  int chunkPotValue = analogRead(chunkPot);
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
      printData();
    }
}

void widthCheck () {
  int widthPotValue = analogRead(widthPot);
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
    printData();
  }
}

void printData() {
  int jpm = chunkAmount * (60000 / (NUM_LEDS * tick));
  double airtime =  (double(100 * (width * chunkAmount))) / (double(NUM_LEDS));
  Serial.println("Current Data:");
  Serial.println("Tick Rate: " + String(tick) + " Chunks: " + String(chunkAmount) + " Width: " + String(width) + " Health: " + String(remainingHealth) );
  Serial.println("Jumps Per Min: " + String(jpm) + " Airtime: " + String(airtime) + "%");
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

void generateHueArray (int startingHealth) {
  if (startingHealth == 1) {
    hue[1] = criticalHue;
    hue[0] = deadHue;
  } else if (startingHealth == 2) {
    hue[2] = highHue;
    hue[1] = criticalHue;
    hue[0] = deadHue;
  } else {
    hue[startingHealth] = highHue;
    hue[2] = lowHue;
    hue[1] = criticalHue;
    hue[0] = deadHue;
    for ( int i = 3; i < startingHealth; i++) {
      hue[i] = (( (i - 2) * (highHue - lowHue) ) / (startingHealth - 2 )) + lowHue;
    }

  }
  // for (int i = 0; i <= startingHealth; i++) {
  //  Serial.print("hue[" + String(i) + "] = " + String(hue[i]) + ", "); // shows the hue[] array in case I choose to debug in the future
  //}
  //Serial.println();
}

// methods below are only relevent if healthOn is true
void healthReduction(bool healthOn) {
  if (healthOn && (remainingHealth > 0)) {
    remainingHealth--;
    if (remainingHealth == 0){
      double increment = 255 / NUM_LEDS;
      for ( int i = 0; i < NUM_LEDS; i++){
        leds[i] = CHSV( increment * i, 255, 255);
      }
      //fill_solid(leds, NUM_LEDS, CHSV( 100.5,255,255));
      FastLED.show();
      playSong();
    }
    printData();
  }
}

CRGB refHueUpdate(){
  //return CRGB( 95, 255 ,255);
  //Serial.println(hue[remainingHealth]); // for periodic full health debugging
  //Serial.println(remainingHealth);
  return CHSV(hue[remainingHealth], 255 ,255);
}

void playSong() { //Beethoven's 5th symphony
    tone(9, NOTE_G2 , 300);
    delay(500);
    tone(9, NOTE_G2 , 300);
    delay(500);
    tone(9, NOTE_G2 , 300);
    delay(500);
    tone(9, NOTE_DS2, 1200);
    delay(2000);
    tone(9, NOTE_F2 , 300);
    delay(500);
    tone(9, NOTE_F2 , 300);
    delay(500);
    tone(9, NOTE_F2 , 300);
    delay(500);
    tone(9, NOTE_D2, 1400);
    delay(2000);
}

