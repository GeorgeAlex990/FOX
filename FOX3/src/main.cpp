#include <Adafruit_SI5351.h> // Clock Generator
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // OLed Display
#include <Encoder.h>

#include <EEPROM.h> // EEPROM library

int address = 0;

/* Setup the OLed display */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_SI5351 clockgen = Adafruit_SI5351();

// Define all the pins
//#define RSSI_PIN A0
#define BUTTON_1 5
#define BUTTON_2 6
//#define BUTTON_3 6
//#define BUTTON_4 7
#define CLK_PIN 3
#define DT_PIN 2
#define SW_PIN 4

#define atenuator 9

int aten[8] = {0, 36, 72, 108, 144, 180, 216, 255};

Encoder Rotary(DT_PIN, CLK_PIN);

/*Setting all the variables*/
float freq;// = 3.5000f;
float eefreq;
float last_freq = 3.5000f;
int freq_dec = 4800;
//int freq_dec = (freq - int(freq)) * 100;
float multiplier = 26.60f;
int multiplier_dec = (multiplier - int(multiplier)) * 100;
int divider = 190;
//int RSSI_LVL = 54; // A value between 0 and 86
//int dist = 999; // A value from 0 to 999 meters showing the distance between the Receiver and the Fox
int MEM_MODE = 0;
bool scan = false;

//
int dec_cursor = 2; // Where is the cursor at freq; From 0 to 3
//

long positionEncoder  = -999;
float test_freq = 3.5000f;
int count = 4;
int SW_state;
int prev_SW_state;
float freq_test_change = 0.5000f;


int currentAten = 0;
bool buttonState = false;
bool lastButtonState = false;
bool buttonState1 = false;
bool lastButtonState1 = false;

float Multiplier_Calc(float freq) {
    multiplier = divider * freq / 25;
    Serial.print("MULTIPLIER = "); Serial.println(freq);

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

void GenerateFreq(float freq) {
  multiplier = Multiplier_Calc(freq);
  multiplier_dec = Multiplier_Dec_Calc(multiplier);

  /* Generate the clock */
  clockgen.setupPLL(SI5351_PLL_A, int(multiplier), multiplier_dec, 100);
  Serial.print("Set Output #0 to "); Serial.print(freq, 4); Serial.print(" MHz, with a multiplier of: "); Serial.println(multiplier);
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

void updateDisplay(float& freq, int dec_cursor) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setFont(NULL);

    display.setTextSize(3);
    freq_dec = (freq - int(freq)) * 10000;
    display.setCursor(5, 5);
    display.print(int(freq));
    display.setCursor(20, 5);
    display.print(".");
    display.setCursor(40, 5);
    display.print(freq_dec);
    display.setCursor(35, 35);
    display.print("MHz");
    display.drawLine((40+(18*dec_cursor)), 2, ((40+(18*dec_cursor))+15), 2, 1);

    display.setTextSize(2);
    display.setCursor(110, 50);
    display.print(currentAten+1);
    
    display.display();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial.println("SI5351 3.4 MHz -> 3.8 MHz"); Serial.println();

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(atenuator, OUTPUT);

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

  EEPROM.get(address, freq);
  Serial.print("FREQ = "); Serial.println(freq);

  if (freq > 4 || freq < 3) {
    freq = 3.5500f;
    EEPROM.put(address, freq);
  }
  
  Serial.print("FREQ = "); Serial.println(freq);


  //freq = 3.4800f;
  updateDisplay(freq, dec_cursor);
}

void loop() {
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(BUTTON_1); // Read the button state

  // Check if the button was pressed (falling edge)
  if (lastButtonState == HIGH && buttonState == LOW) {
    currentAten++; // Advance the position in the array
    if (currentAten >= 8) { // Loop back to 0 if we go past the end
      currentAten = 0;
    }
    Serial.print("Current Value: ");
    Serial.println(aten[currentAten]); // Print the current array value

    analogWrite(atenuator, aten[currentAten]);
  }

  lastButtonState = buttonState;

  buttonState1 = digitalRead(BUTTON_2); // Read the button state

  // Check if the button was pressed (falling edge)
  if (lastButtonState1 == HIGH && buttonState1 == LOW) {
    currentAten--; // Advance the position in the array
    if (currentAten < 0) { // Loop back to 0 if we go past the end
      currentAten = 7;
    }
    Serial.print("Current Value: ");
    Serial.println(aten[currentAten]); // Print the current array value

    analogWrite(atenuator, aten[currentAten]);
  }

  lastButtonState1 = buttonState1;

  test_freq = freq;
    SW_state = digitalRead(SW_PIN);

    if (SW_state != prev_SW_state) {
        count++;
        Rotary.write(0); // Reset encoder to 0
        if (count > 6) {
            count = 4;
        }
        delay(50); // Debounce
    }

    prev_SW_state = SW_state;
    dec_cursor = count / 2;

    long newEncoder = Rotary.read() / 4; // Ensure one step per detent
    if (newEncoder != positionEncoder) {
        positionEncoder = newEncoder;
        float stepSizes[] = {0.001, 0.001, 0.001, 0.0001}; // Adjust these as needed
        test_freq = freq + newEncoder * stepSizes[dec_cursor];
        Rotary.write(0);

        // Constrain the frequency within desired limits
        if (test_freq < 3.5) test_freq = 3.5;
        if (test_freq > 4.0) test_freq = 4.0;

        Serial.print("Frequency: ");
        Serial.println(test_freq, 4);
    }

    freq = test_freq;
  
  updateDisplay(freq, dec_cursor);
  
  //Serial.print("FREQ = "); Serial.println(freq, 4);

  if (freq != last_freq) {
    last_freq = freq;
    GenerateFreq(freq);
  }

  EEPROM.get(address, eefreq);
  Serial.println(eefreq, 4);
  if (freq - eefreq > 0.005 || freq - eefreq < -0.005) {
    //EEPROM.update(address, freq);
    if (EEPROM.read(address) != freq) {
      EEPROM.put(address, freq);
      Serial.println("Editat");
    }
  }
}