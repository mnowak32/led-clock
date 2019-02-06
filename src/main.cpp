#include <Arduino.h>
#include <NeoPixelBrightnessBus.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <TimerOne.h>

const uint16_t PixelCount = 60 + 24;
NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, 4);

ThreeWire myWire(11, 10, 12); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

RgbColor
  rgbClear = RgbColor(0, 0, 0),
  rgbHrMark = RgbColor(0, 0, 80),
  rgbMinMark = RgbColor(0, 80, 0),
  rgbSecMark = RgbColor(80, 0, 0),
  rgbHrDot1 = RgbColor(25, 0, 0),
  rgbHrDot2 = RgbColor(20, 15, 0),
  rgbMinDot1 = RgbColor(0, 0, 15),
  rgbMinDot2 = RgbColor(0, 20, 15)
  ;

//current time
volatile int h = 8;
volatile int m = 37;
volatile int s = 12;

volatile int readTime = 1; //read time on the first loop

RgbColor rgbAdd(RgbColor c1, RgbColor c2) {
  return RgbColor(
    (c1.R + c2.R),
    (c1.G + c2.G),
    (c1.B + c2.B)
  );
}

#define MIN_OFFSET 7

void showHour() {
  strip.ClearTo(rgbClear);
  
  //hour
  // select two neighbouring pixels - one for the hour and the next for hour + 1
  int hp1id = h * 2 + (m / 30); // full hour (*2) plus 1 if there is more than half past the hour
  int hp2id = (hp1id + 1) % 24; // wrap around past 24th pixel
  for (int p = 0; p < 24; p++) {
    //every sixth pixel (at 12, 3, 6 & 9 hours) has base color changed to rgbHrDot1
    //every even pixel (besides the four) marks a whole hour and is rgbHrDot2
    RgbColor base = (p % 6 == 0) ? rgbHrDot1 : (p % 2 == 0) ? rgbHrDot2 : rgbClear;
    if (p == hp1id) {
      int after = m % 30;
      RgbColor pixCol = rgbHrMark;
      pixCol.Darken((after * after) / 4);
      strip.SetPixelColor(p, rgbAdd(pixCol, base));
    } else if (p == hp2id) {
      int before = 30 - m % 30;
      RgbColor pixCol = rgbHrMark;
      pixCol.Darken((before * before) / 4);
      strip.SetPixelColor(p, rgbAdd(pixCol, base));
    } else {
      strip.SetPixelColor(p, base);
    }
  }

  //minute & second
  int mp1id = m;
  int mp2id = (mp1id + 1) % 60;
  for (int p = 0; p < 60; p++) {
    int po = (p + MIN_OFFSET) % 60;

    RgbColor base = (p == 0) ? rgbMinDot2 : (p % 5 == 0) ? rgbMinDot1 : rgbClear;
    if (p == s) { //show seconds dot
      base = rgbAdd(base, rgbSecMark);
    }

    if (p == mp1id) {
      RgbColor pixCol = rgbMinMark;
      pixCol.Darken((s * s) / 15);
      strip.SetPixelColor(po + 24, rgbAdd(pixCol, base));
    } else if (p == mp2id) {
      int before = 60 - s;
      RgbColor pixCol = rgbMinMark;
      pixCol.Darken((before * before) / 15);
      strip.SetPixelColor(po + 24, rgbAdd(pixCol, base));
    } else {
      strip.SetPixelColor(po + 24, base);
    }
  }

  strip.Show();
}

void addSecond() {
  s++;
  if (s > 59) {
    s = 0;
    m++;
    if (m > 59) {
      m = 0;
      h = (h + 1) % 12;
    }
  }

  if (s == 5 || s == 20 || s == 45) {
    readTime = 1;
  }
}

void doReadTime() {
  RtcDateTime now = Rtc.GetDateTime();
  h = (now.Hour() % 12);
  m = now.Minute();
  s = now.Second();
}

void setup() {
  strip.Begin();
  strip.SetBrightness(20);

  Serial.begin(57600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (Rtc.GetIsWriteProtected()) {
      Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
      Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }  

  Timer1.initialize();
  Timer1.attachInterrupt(addSecond, 1000000);
}

void loop() {
    showHour();
    delay(150);
    if (readTime) {
      doReadTime();
      readTime = 0;
    }
}