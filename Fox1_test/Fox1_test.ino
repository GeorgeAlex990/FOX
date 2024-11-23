#include <Adafruit_SI5351.h> // Clcok Generator
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // OLed Display
#include <Encoder.h>

/* Setup the OLed display */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_SI5351 clockgen = Adafruit_SI5351();

// Define all the pins
#define RSSI_PIN A0
#define BUTTON_1 4
#define BUTTON_2 5
#define BUTTON_3 6
#define BUTTON_4 7
#define CLK_PIN 3
#define DT_PIN 2
#define SW_PIN 9

Encoder Rotary(DT_PIN, CLK_PIN);

/*Setting all the variables*/
float freq = 3.5000f;
int freq_dec = (freq - int(freq)) * 100;
float multiplier = 26.60f;
int multiplier_dec = (multiplier - int(multiplier)) * 100;
int divider = 190;
//int RSSI_LVL = 54; // A value between 0 and 86
//int dist = 999; // A value from 0 to 999 meters showing the distance between the Receiver and the Fox
int MEM_MODE = 0;
bool scan = false;
int dec_cursor = 0; // Where is the cursor at freq; From 0 to 3

long positionEncoder  = -999;
float test_freq = 3.5000;
int count = 0;
int SW_state;
int prev_SW_state;
float freq_test_change = 0.5000;

float M1 = 3.5000, M2 = 3.6000, M3 = 3.5500, M4 = 3.5826;
int lastMEM = 1;

float Multiplier_Calc(float freq) {
    multiplier = divider * freq / 25;

    if (multiplier > 36) {
      multiplier = 36;
    } // Limit the multiplier, between 24 and 36 to result a freqency between 600 to 900 MHz
    if (multiplier < 24) {
      multiplier = 24;
    }
    return multiplier;
}

int Multiplier_Dec_Calc(float multiplier) {
    multiplier_dec = (multiplier - int(multiplier)) * 100;
    return multiplier_dec;
}

void GenerateFreq(int freq) {
  multiplier = Multiplier_Calc(freq);
  multiplier_dec = Multiplier_Dec_Calc(multiplier);

  /* Generate the clock */
  clockgen.setupPLL(SI5351_PLL_A, int(multiplier), multiplier_dec, 100);
  //Serial.print("Set Output #0 to "); Serial.print(freq, 4); Serial.print(" MHz, with a multiplier of: "); Serial.println(multiplier);
  clockgen.setupMultisynth(0, SI5351_PLL_A, divider, 0, 100);
}

void StartUp() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setFont(NULL);
  display.setTextSize(1);
  display.setCursor(40, 16);
  display.println("MADE BY");
  display.setTextSize(2);
  display.setCursor(26, 30);
  display.println("YO2GAR");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
}

void updateDisplay(float& freq, int dec_cursor, int RSSI_LVL, bool scan, int MEM_MODE, int dist) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setFont(NULL);

    /* Setup for RSSI */
    display.setTextSize(1);
    display.setCursor(5, 40);
    display.print("RSSI");
    display.drawRect(30, 38, 90, 11, 1);
    display.fillRect(32, 40, RSSI_LVL, 7, 1);

    /* Draw Memory buttons and the cursor to select them */
    display.setCursor(5, 55);
    display.print("MEM1");
    display.setCursor(35, 55);
    display.print("MEM2");
    display.setCursor(65, 55);
    display.print("MEM3");
    display.setCursor(95, 55);
    display.print("MEM4");
    if (MEM_MODE == 0) {
      display.drawLine(5, 52, 27, 52, 0);
      display.drawLine(35, 52, 58, 52, 0);
      display.drawLine(65, 52, 88, 52, 0);
      display.drawLine(95, 52, 118, 52, 0);
      freq = test_freq;
    }
    else if (MEM_MODE == 1) {
      display.drawLine(5, 52, 27, 52, 1);
      freq = M1;
    }
    else if (MEM_MODE == 2) {
      display.drawLine(35, 52, 58, 52, 1);
      freq = M2;
    }
    else if (MEM_MODE == 3) {
      display.drawLine(65, 52, 88, 52, 1);
      freq = M3;
    }
    else if (MEM_MODE == 4) {
      display.drawLine(95, 52, 118, 52, 1);
      freq = M4;
    }
    else {
      freq = freq;
    }
    
    /* Draw the Scan Button */
    display.setTextSize(1);
    display.setCursor(5, 25);
    display.print("SCAN");
    if (scan) {
      display.drawRect(3, 23, 27, 11, 1);
    }
    else {
      display.drawRect(3, 23, 27, 11, 0);
    }
    //scan = false;

    /* Draw the distance to the Fox */
    display.setCursor(50, 25);
    display.print("DIST:");
    display.setCursor(85, 25);
    display.print(dist);
    display.setCursor(110, 25);
    display.print("m");

    /* Setup for freq display */
    display.setTextSize(2);
    freq_dec = (freq - int(freq)) * 10000;
    display.setCursor(5, 5);
    display.println(int(freq));
    display.setCursor(14, 5);
    display.println(".");
    display.setCursor(25, 5);
    display.print(freq_dec);
    display.setCursor(90, 5);
    display.print("MHz");
    display.drawLine((25+(12*dec_cursor)), 2, ((25+(12*dec_cursor))+10), 2, 1);
    
    display.display();
}

int LVL_raw = 0;
int RSSI_LVL_Calc() {
  LVL_raw = analogRead(RSSI_PIN);
  return map(LVL_raw, 0, 676, 0, 86);
}

int Distance_Calc(int RSSI_LVL) {
  return map(RSSI_LVL, 0, 86, 999, 0);
}

int MEM_MODE_SELECTOR() {
  if (!digitalRead(BUTTON_1)) {
    lastMEM = 1;
    return 1;
  }
  else if (!digitalRead(BUTTON_2)) {
    lastMEM = 2;
    return 2;
  }
  else if (!digitalRead(BUTTON_3)) {
    lastMEM = 3;
    return 3;
  }
  else if (!digitalRead(BUTTON_4)) {
    lastMEM = 4;
    return 4;
  }
  else if (digitalRead(BUTTON_4) && digitalRead(BUTTON_3) && digitalRead(BUTTON_2) && digitalRead(BUTTON_1)) {
    lastMEM = 0;
    return 0;
  }
  else {
    return lastMEM;
  }
}

float SCAN_FREQ(/*float freq_scanned*/) {
  //display.drawRect(3, 23, 27, 11, 1);
  bool scan_control = true;
  float freq_scanned = 3.450;
  while (scan_control) {
    if (RSSI_LVL_Calc() > 30) {
      scan_control = false;
      display.drawRect(3, 23, 27, 11, 0);
    }
    else if (!digitalRead(BUTTON_2) && !digitalRead(BUTTON_4)) {
      scan_control = false;
      display.drawRect(3, 23, 27, 11, 0);
    }
    freq_scanned += 0.001;
    if (freq_scanned >= 3.650) {
      freq_scanned = 3.450;
    }
    Serial.println(freq_scanned, 4);

    GenerateFreq(freq_scanned);
    
    updateDisplay(freq, dec_cursor, RSSI_LVL_Calc(), scan, MEM_MODE, Distance_Calc(RSSI_LVL_Calc()));
    display.display();
  }

  return freq_scanned;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial.println("SI5351 3.4 MHz -> 3.8 MHz"); Serial.println();

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  pinMode(RSSI_PIN, INPUT);
  pinMode(SW_PIN, INPUT);

  prev_SW_state = digitalRead(SW_PIN);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  /* Initialise the sensor */
  if (clockgen.begin() != ERROR_NONE)
  {
    /* There was a problem detecting the IC ... check your connections */
    //Serial.print("Ooops, no Si5351 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  //Serial.println("Set PLLA to 600MHz");
  clockgen.setupPLL(SI5351_PLL_A, int(multiplier), multiplier_dec, 100);
  //Serial.print("Set Output #0 to "); Serial.print(freq, 4); Serial.print(" MHz, with a multiplier of: "); Serial.println(multiplier);
  clockgen.setupMultisynth(0, SI5351_PLL_A, divider, 0, 100);

  clockgen.enableOutputs(true);

  StartUp();
  updateDisplay(freq, dec_cursor, RSSI_LVL_Calc(), scan, MEM_MODE, Distance_Calc(RSSI_LVL_Calc()));
}

void loop() {
  // put your main code here, to run repeatedly:

  SW_state = digitalRead(SW_PIN);
  if (SW_state != prev_SW_state) {
    count++;
    Rotary.write(0);
    freq_test_change = (test_freq - 3) * 10000;
    if (count > 6) {
      count = 0;
    }
    delay(50);
  }
  prev_SW_state = SW_state;
  dec_cursor = count / 2;
  //Serial.print("Count "); Serial.println(dec_cursor);

  long newEncoder;
  newEncoder = Rotary.read()/4;
  if (newEncoder != positionEncoder) {
    Serial.print("Left = ");
    Serial.print(newEncoder);
    Serial.println();
    positionEncoder = newEncoder;
    test_freq = 3 + (freq_test_change + newEncoder * pow(10, 3-dec_cursor)) / 10000.0;
  
    Serial.print("freq "); Serial.println(test_freq, 4);
    //freq = test_freq;
  }
  else {
    freq = test_freq;
  }
  
  MEM_MODE = MEM_MODE_SELECTOR();
  if (!digitalRead(BUTTON_1) && !digitalRead(BUTTON_4)) {
    scan = true;
    MEM_MODE = 0;
    updateDisplay(freq, dec_cursor, RSSI_LVL_Calc(), scan, MEM_MODE, Distance_Calc(RSSI_LVL_Calc()));
    freq = SCAN_FREQ(/*freq*/);
    scan = false;
  }
  if (!digitalRead(BUTTON_1) && !digitalRead(SW_PIN)) {
    M1 = freq;
  }
  if (!digitalRead(BUTTON_2) && !digitalRead(SW_PIN)) {
    M2 = freq;
  }
  if (!digitalRead(BUTTON_3) && !digitalRead(SW_PIN)) {
    M3 = freq;
  }
  if (!digitalRead(BUTTON_4) && !digitalRead(SW_PIN)) {
    M4 = freq;
  }
  updateDisplay(freq, dec_cursor, RSSI_LVL_Calc(), scan, MEM_MODE, Distance_Calc(RSSI_LVL_Calc()));

  GenerateFreq(freq);
}
