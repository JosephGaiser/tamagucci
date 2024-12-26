#define USE_TFT_ESPI_LIBRARY
#define NUMPIXELS 1
#define TOUCH_CS 15 // Define the TOUCH_CS pin

#include <TFT_eSPI.h>
#include <SPI.h>
#include <I2C_BM8563.h>
#include <Adafruit_NeoPixel.h>  
#include "lv_xiao_round_screen.h"
#include "face_smile.cpp"
#include "face_smile_b.cpp"
#include "face_frown.cpp"
#include "face_frown_b.cpp"
#include "tImage.h"

const int Power = 11;
const int PIN = 12;
const int chipSelect = D2;
const int blinkInterval = 50;
const int blinkDuration = 2;
const long interval = 20;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
TFT_eSprite sprite = TFT_eSprite(&tft);
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);

bool is_happy = true;
int blinkCounter = 0;
int randomBlinkInterval = blinkInterval;
unsigned long previousMillis = 0;
bool wasTouched = false;

bool showTime = false;
unsigned long lastTapTime = 0;
const unsigned long doubleTapThreshold = 300; // 300 milliseconds for double-tap

void setup() {
    pixels.begin();
    pinMode(Power, OUTPUT);
    digitalWrite(Power, HIGH);

    tft.init();
    tft.setRotation(0);
    randomSeed(analogRead(0));
    init_rtc();
}

void updateBlinkCounter() {
    blinkCounter++;
    if (blinkCounter >= randomBlinkInterval + blinkDuration) {
        blinkCounter = 0;
        randomBlinkInterval = blinkInterval + random(0, 20);
    }
}

void updateBrightness() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        float brightness = (sin(currentMillis / 5000.0 * 2 * PI) + 1) / 2 * 255;
        if (is_happy) {
            pixels.setPixelColor(0, pixels.Color(0, brightness, 0));
        } else {
            pixels.setPixelColor(0, pixels.Color(brightness, 0, 0));
        }
        pixels.show();
    }
}

void displayImage(const tImage& image, const tImage& blinkImage) {
    if (blinkCounter < randomBlinkInterval || blinkCounter >= randomBlinkInterval + blinkDuration) {
        sprite.pushImage(0, 0, image.width, image.height, (uint16_t*)image.data);
    } else {
        sprite.pushImage(0, 0, blinkImage.width, blinkImage.height, (uint16_t*)blinkImage.data);
    }
}

void loop() {
    get_touch();
    sprite.createSprite(240, 240);
    if (showTime) {
        displayTime();
    } else {
        if (is_happy) {
            displayImage(face_smile, face_smile_b);
        } else {
            displayImage(face_frown, face_frown_b);
        }
    }
    sprite.pushSprite(0, 0);
    sprite.deleteSprite();

    updateBlinkCounter();
    updateBrightness();
}

void get_touch() {
    lv_coord_t touchX, touchY;
    if (chsc6x_is_pressed()) {
        unsigned long currentTapTime = millis();
        if (currentTapTime - lastTapTime < doubleTapThreshold) {
            showTime = !showTime;
            lastTapTime = 0; // Reset last tap time to avoid multiple toggles
        } else {
            lastTapTime = currentTapTime;
        }
        wasTouched = true;
    } else {
        wasTouched = false;
    }
}

void displayTime() {
    I2C_BM8563_TimeTypeDef time;
    rtc.getTime(&time);
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", time.hours, time.minutes, time.seconds);
    sprite.fillSprite(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(3);
    sprite.drawString(timeStr, 60, 110);
    sprite.pushSprite(0, 0);
}

void init_rtc() {
    Wire.begin();
    rtc.begin();

    I2C_BM8563_TimeTypeDef time;
    rtc.getTime(&time);

    // Check if the RTC is running by verifying if the time is valid
    if (time.hours == 0 && time.minutes == 0 && time.seconds == 0) {
        I2C_BM8563_DateTypeDef date = {01, 01, 25}; //  MM DD YY
        I2C_BM8563_TimeTypeDef time = {00, 00, 00};
        rtc.setDate(&date);
        rtc.setTime(&time);
    }
}