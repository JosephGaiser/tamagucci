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

#define USE_TFT_ESPI_LIBRARY
#define NUMPIXELS 1

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

void setup() {
    pixels.begin();
    pinMode(Power, OUTPUT);
    digitalWrite(Power, HIGH);

    tft.init();
    tft.setRotation(0);
    randomSeed(analogRead(0));
    init_rtc();
}

void addNoise(uint16_t* imageData, int width, int height, int noiseLevel) {
    for (int i = 0; i < width * height; ++i) {
        if (random(0, 100) < noiseLevel) {
            imageData[i] ^= 0xFFFF;
        }
    }
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
        addNoise((uint16_t*)image.data, image.width, image.height, 5);
        sprite.pushImage(0, 0, image.width, image.height, (uint16_t*)image.data);
    } else {
        addNoise((uint16_t*)blinkImage.data, blinkImage.width, blinkImage.height, 5);
        sprite.pushImage(0, 0, blinkImage.width, blinkImage.height, (uint16_t*)blinkImage.data);
    }
}

void loop() {
    get_touch();
    sprite.createSprite(240, 240);
    if (is_happy) {
        displayImage(face_smile, face_smile_b);
    } else {
        displayImage(face_frown, face_frown_b);
    }
    sprite.pushSprite(0, 0);
    sprite.deleteSprite();

    updateBlinkCounter();
    updateBrightness();
}

void get_touch() {
    lv_coord_t touchX, touchY;
    if (chsc6x_is_pressed()) {
        if (!wasTouched) {
            chsc6x_get_xy(&touchX, &touchY);
            if (touchX > 240 || touchY > 240) {
                touchX = 0;
                touchY = 0;
            }
            is_happy = !is_happy;
            wasTouched = true;
        }
    } else {
        wasTouched = false;
    }
}

void init_rtc() {
    Wire.begin();
    rtc.begin();
    I2C_BM8563_DateTypeDef date = {11, 7, 2024};
    I2C_BM8563_TimeTypeDef time = {18, 30, 20};
    rtc.setDate(&date);
    rtc.setTime(&time);
}