/*
  ArduinoOscilloscope
  Implementaiton of the simple Oscilloscope on Arduino Mini. The inspiration came from http://srukami.inf.ua/pultoscop_v1_1.html
  However, completely rebuilt and fitted to TFT ILI9163C display and including automated scaling
*/

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <SPI.h>
#include <FreqCount.h>
#define DISPLAY_WIDTH 128 
#define DISPLAY_HEIGHT 128
#define ARDUINO_PRECISION 1023.0

#ifdef __AVR__
#define sinf sin
#endif

/* some RGB color definitions                                                 */
#define Black           0x0000      /*   0,   0,   0 */
#define Navy            0x000F      /*   0,   0, 128 */
#define DarkGreen       0x03E0      /*   0, 128,   0 */
#define DarkCyan        0x03EF      /*   0, 128, 128 */
#define Maroon          0x7800      /* 128,   0,   0 */
#define Purple          0x780F      /* 128,   0, 128 */
#define Olive           0x7BE0      /* 128, 128,   0 */
#define LightGrey       0xC618      /* 192, 192, 192 */
#define DarkGrey        0x7BEF      /* 128, 128, 128 */
#define Blue            0x001F      /*   0,   0, 255 */
#define Green           0x07E0      /*   0, 255,   0 */
#define Cyan            0x07FF      /*   0, 255, 255 */
#define Red             0xF800      /* 255,   0,   0 */
#define Magenta         0xF81F      /* 255,   0, 255 */
#define Yellow          0xFFE0      /* 255, 255,   0 */
#define White           0xFFFF      /* 255, 255, 255 */
#define Orange          0xFD20      /* 255, 165,   0 */
#define GreenYellow     0xAFE5      /* 173, 255,  47 */
#define Pink                        0xF81F

#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define NDOTS 512			// Number of dots 512
#define SCALE 4096//4096
#define INCREMENT 512//512
#define PI2 6.283185307179586476925286766559
#define RED_COLORS (32)
#define GREEN_COLORS (64)
#define BLUE_COLORS (32)


#define FASTADC 1
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

TFT_ILI9163C display = TFT_ILI9163C(10, 9, 8);

boolean autoHScale = true; // Automatic horizontal (time) scaling
byte thresLocation = 0; // Threshold bar location
int channelAI = A4;      //Signal iput pun
int PINakum = A6;      //Battery measumenent pin
int delayAI = A5;       //Divider on the variable resistor
int razv = 32;// Screen scan

#define DELAY_POTENTIMETER
#ifdef DELAY_POTENTIMETER
#endif

float avgV = 0;
float tmpV = 0;
float delayVariable = 1000;
float scale = 0;
int xCounter = 0;
int yPosition = 0;
int readings[DISPLAY_WIDTH + 1];
int counter = 0;
unsigned long drawtime = 0;
unsigned long lastdraw = 0;
unsigned long count = 0;
int frames = 0;

//byte theThreshold = 128; // 0-255, Multiplied by voltageConst
float maxV = 0.0;
float minV = 0.0;
float ptopV = 5.0; //also a scal

int plothHscale = 1;

void setup()
{
  Serial.begin(115200);

#if FASTADC
  // set prescale to 16
  sbi(ADCSRA, ADPS2) ;
  cbi(ADCSRA, ADPS1) ;
  cbi(ADCSRA, ADPS0) ;
#endif
  delay(100);
  FreqCount.begin(1000);
  display.begin();

  display.setRotation(4);
  display.fillScreen(BLACK);
  display.setTextWrap(true);
  display.setTextColor(WHITE, BLACK);

  display.display(1);
  display.clearScreen();
  display.setCursor(5, 0);
  display.print("OSCILLOSCOPE");
  display.setCursor(10, 10);
  display.print("Mobile v1.0");
  /////////////////// Battery Test  /////////////////// 
  if ((analogRead(PINakum) / ARDUINO_PRECISION * 5.0) > 3.7) {
    display.setCursor(5, 25);
    display.print("Test Batt OK!");
  } else if ((analogRead(PINakum) / ARDUINO_PRECISION * 5.0) < 3.3) {
    display.setCursor(5, 25);
    display.print("Test Batt NO!");
  } else
  {
    display.setCursor(5, 25);
    display.print("Test Batt LOW!");
  }

  display.setCursor(25, 35);
  display.print(analogRead(PINakum) / ARDUINO_PRECISION * 5.0);
  display.print("V");
  display.display(1);
  delay(1000);
  display.clearScreen();
}



void loop()
{
  long t0, t;
  avgV = 0;
  scale = (float)(DISPLAY_HEIGHT - 16) / ARDUINO_PRECISION;
  /////////////////// Dividing scale according to signal  /////////////////// 
  t0 = micros();
  for (xCounter = 0; xCounter < DISPLAY_WIDTH - 2 * 8; xCounter++)
  {
    readings[xCounter] = analogRead(channelAI);
#ifdef DELAY_POTENTIMETER
    //delay (delayVariable);
    delayMicroseconds(delayVariable);
#endif
  }
  t = micros() - t0; // calculate elapsed time
  Serial.print("Time per sample: ");
  Serial.println((float)(t - delayVariable * (DISPLAY_WIDTH - 2 * 8)) / (DISPLAY_WIDTH - 2 * 8));
  Serial.print("Delay: ");
  Serial.println(delayVariable);
  // Maximum Voltage
  maxV = -100;
  // Minimum Voltage
  minV = 100;

  for (xCounter = 0; xCounter < DISPLAY_WIDTH - 2 * 8; xCounter++)
  {
    readings[xCounter] = readings[xCounter] * scale;

    // Average Voltage
    tmpV = readings[xCounter] * 5.0 / (float)(DISPLAY_WIDTH - 16);
    avgV = avgV + tmpV;
    if (tmpV > maxV) maxV = tmpV;
    if (tmpV < minV) minV = tmpV;

  }
  avgV = avgV / (DISPLAY_WIDTH - 2 * 8);

  // Peak-to-Peak Voltage
  ptopV = maxV - minV;
  if (ptopV < 0.1) {
    ptopV = 0.1; //to avoid empty plot
    minV = avgV - ptopV / 2;
    maxV = avgV + ptopV / 2;
  }

  Serial.print("ptopV: ");
  Serial.println(ptopV);

  Serial.print("maxV: ");
  Serial.println(maxV);

  Serial.print("minV: ");
  Serial.println(minV);

  Serial.print("avgV: ");
  Serial.println(avgV);


#ifdef DELAY_POTENTIMETER

  if (delayVariable > 16383)
    delayVariable = 16383;

  if (autoHScale == true) {
    // With automatic horizontal (time) scaling enabled,
    // scale quickly if the threshold location is far, then slow down
    if (thresLocation > 15 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 500;
    } else if (thresLocation < (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 500;
    }  else  if (thresLocation > 14 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 200;
    } else if (thresLocation < 2 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 200;
    }   else  if (thresLocation > 12 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 100;
    } else if (thresLocation < 4 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 100;
    }   else  if (thresLocation > 11 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 50;
    } else if (thresLocation < 5 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 50;
    }   else  if (thresLocation > 10 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 10;
    } else if (thresLocation < 6 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 10;
    } else if (thresLocation > 8 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable + 1;
    } else if (thresLocation < 8 * (DISPLAY_WIDTH - 2 * 8) / 16) {
      delayVariable = delayVariable - 1;
    }
  }
  thresLocation = 1;
  while ((readings[thresLocation] > avgV) && (thresLocation < (DISPLAY_WIDTH - 2 * 8)))
    (thresLocation++);
  thresLocation++;
  while ((readings[thresLocation] < avgV) && (thresLocation < (DISPLAY_WIDTH - 2 * 8)))
    (thresLocation++);
  Serial.print("thresLocation: ");
  Serial.println(thresLocation);

  // Enforce minimum time periods
  if (delayVariable < 20) {
    delayVariable = 20;
  }

#endif

  char str[2];     //result string 5 positions + \0 at the end
  // convert float to fprintf type string
  // format 5 positions with 3 decimal places

  display.setTextColor(BLACK, WHITE);
  display.fillRect(2 * 8 + 2, 8, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 16, BLACK);
  //##############################################Voltage scale

  display.setCursor(0, ((DISPLAY_HEIGHT - 8) - (.2 * ARDUINO_PRECISION * scale)));
  display.print(dtostrf(minV + ptopV * 0.2, 1, 1, str ));
  display.setCursor(0, ((DISPLAY_HEIGHT - 8) - (.4 * ARDUINO_PRECISION * scale)));
  display.print(dtostrf(minV + ptopV * 0.4, 1, 1, str ));
  display.setCursor(0, ((DISPLAY_HEIGHT - 8) - (.6 * ARDUINO_PRECISION * scale)));
  display.print(dtostrf(minV + ptopV * 0.6, 1, 1, str ));
  display.setCursor(0, ((DISPLAY_HEIGHT - 8) - (.8 * ARDUINO_PRECISION * scale)));
  display.print(dtostrf(minV + ptopV * 0.8, 1, 1, str ));
  display.setCursor(0, ((DISPLAY_HEIGHT - 8) - (1.0 * ARDUINO_PRECISION * scale)));
  display.print(dtostrf(minV + ptopV * 1.0, 1, 1, str ));
  //##############################################Signal drawing
  if ( thresLocation < (DISPLAY_WIDTH - 2 * 8) / 4)
    plothHscale = floor((DISPLAY_WIDTH - 2 * 8) / thresLocation / 2);
  else
    plothHscale = 1;

  for (xCounter = 0; xCounter < ceil((DISPLAY_WIDTH - 2 * 8) / plothHscale); xCounter++)
  {
    display.drawPixel(2 * 8 + 2 + (xCounter * plothHscale), (DISPLAY_HEIGHT - 8) - (readings[xCounter] - (float)scale * ARDUINO_PRECISION * minV / 5.0) * 5.0 / ptopV, BLUE);
    Serial.print(" reading: ");
    Serial.println(readings[xCounter]);
    Serial.print("pixel: ");
    Serial.println((DISPLAY_HEIGHT - 8) - (readings[xCounter] - (float)scale * ARDUINO_PRECISION * minV / 5.0) * 5.0 / ptopV);
    Serial.print("minn: ");
    Serial.println(((float)scale * ARDUINO_PRECISION * minV / 5.0));

    if (xCounter > 1) {
      display.drawLine(2 * 8 + 2 + ((xCounter - 1)*plothHscale), (DISPLAY_HEIGHT - 8) - (readings[xCounter - 1] - (float)scale * ARDUINO_PRECISION * minV / 5.0) * 5.0 / ptopV, 2 * 8 + 2 + (xCounter * plothHscale), (DISPLAY_HEIGHT - 8) - (readings[xCounter] - (float)scale * ARDUINO_PRECISION * minV / 5.0) * 5.0 / ptopV, BLUE);
    }
  }

  display.setTextColor(BLACK, GREEN);

  /////////////////// Frequency output, considering KHz  /////////////////// 
  display.fillRect(0, 0, DISPLAY_WIDTH - 1, 8, GREEN);
 
  display.setCursor(0, DISPLAY_HEIGHT - 7);
  display.print("       ");
  display.setCursor(80, DISPLAY_HEIGHT - 7);
  display.print("       ");
  display.setCursor(56, DISPLAY_HEIGHT - 7);
  display.print("   ");

  if (count < 10000) {
    display.setCursor(32, 0);
    display.print(count);
    display.print("Hz");
  }
  if (count > 10000) {
    display.setCursor(32, 0);
    display.print((count) / 1000);
    display.print("KHz");
  }

  display.setCursor(72, 0);
  display.print("Av");
  display.print(round(avgV));
  display.print("V");

  display.setCursor(0, DISPLAY_HEIGHT - 7);
  display.print("Sc");
  if (razv >= 1000)  {
    display.print(round(razv / 1000));
    display.print("s");
  } else {
    display.print(round(razv));
    display.print("ms");
  }

  display.setCursor(56, DISPLAY_HEIGHT - 7);
  if (plothHscale > 1)
  {
    display.setTextColor(BLACK, Orange);

  }
  display.print("H");
  display.print(plothHscale);
  if (plothHscale < 10)
    display.print(" ");

  display.setTextColor(BLACK, GREEN);

  display.setCursor(80, DISPLAY_HEIGHT - 7);
  display.print("Sp");
  if (delayVariable >= 1000)  {
    display.print(round(delayVariable / 1000));
    display.print("ms");
  } else {
    display.print(round(delayVariable));
    display.print("us");
  }

  /////////////////// Printing voltage  /////////////////// 
  display.setCursor(0, 0);
  display.print(analogRead(channelAI) / ARDUINO_PRECISION * 5.0);
  display.print("V");
  display.display(1);

  //Print battery level
  display.setCursor(102, 0);
  short int batVal = round( (analogRead(PINakum) / ARDUINO_PRECISION * 5.0 - 3.3) * 100 / 0.9);
  (batVal > 0) ? batVal = batVal : batVal = 0;
  display.print(batVal);
  display.print("%");

  //##############################################Frequency calculation
  if (FreqCount.available())
    count = FreqCount.read();

  if (digitalRead(3) == HIGH) // Button state
  {
    razv = razv * 2;
    if (razv > 1000) // Button state
      razv = 1000;
    if (razv == 0)
      razv = 1;
    delay(3);//Protection from contact bounce
  }
  if (digitalRead(2) == HIGH) // Button state
  {
    razv = round(razv / 2);
    if (razv == 1) // Button state
      razv = 1;
    delay(3);//Protection from contact bounce
  }

  delay(razv);//Adjustment of the sampling period
  // may be delayMicroseconds()?
}
