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

bool showTime = true;
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
I2C_BM8563_DateTypeDef currentDate;
I2C_BM8563_TimeTypeDef currentTime;
bool settingTime = false;
int settingStep = 0;

void displayTimeSettingScreen() {
    sprite.createSprite(240, 240);
    sprite.fillSprite(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(2);

    // Display current value based on settingStep
    char timeStr[11];
    switch (settingStep) {
        case 0: sprintf(timeStr, "Month: %02d", currentDate.month); break;
        case 1: sprintf(timeStr, "Day: %02d", currentDate.date); break;
        case 2: sprintf(timeStr, "Year: %04d", currentDate.year); break; // Updated to %04d
        case 3: sprintf(timeStr, "Hour: %02d", currentTime.hours); break;
        case 4: sprintf(timeStr, "Min: %02d", currentTime.minutes); break;
        case 5: sprintf(timeStr, "Sec: %02d", currentTime.seconds); break;
    }
    sprite.drawString(timeStr, 60, 110);

    // Draw left and right arrows
    sprite.fillTriangle(30, 120, 50, 100, 50, 140, TFT_WHITE); // Left arrow
    sprite.fillTriangle(210, 120, 190, 100, 190, 140, TFT_WHITE); // Right arrow

    // Draw confirm button
    sprite.fillRect(90, 210, 60, 20, TFT_WHITE);
    sprite.setTextColor(TFT_BLACK);
    sprite.drawString("Confirm", 95, 215);

    sprite.pushSprite(0, 0);
    sprite.deleteSprite();
}

void updateSettingValue(bool increase) {
    switch (settingStep) {
        case 0:
            currentDate.month = max(1, min(12, currentDate.month + (increase ? 1 : -1)));
            break;
        case 1:
            currentDate.date = max(1, min(31, currentDate.date + (increase ? 1 : -1)));
            break;
        case 2:
            currentDate.year = max(2024, min(2099, currentDate.year + (increase ? 1 : -1))); // Updated range
            break;
        case 3:
            currentTime.hours = max(0, min(23, currentTime.hours + (increase ? 1 : -1)));
            break;
        case 4:
            currentTime.minutes = max(0, min(59, currentTime.minutes + (increase ? 1 : -1)));
            break;
        case 5:
            currentTime.seconds = max(0, min(59, currentTime.seconds + (increase ? 1 : -1)));
            break;
    }
}

const unsigned long longPressThreshold = 2000; // 1000 milliseconds for long press
unsigned long touchStartTime = 0;
lv_coord_t touchX = 0, touchY = 0; // Add variables to store touch coordinates
const unsigned long touchCooldown = 500; // 500 milliseconds cooldown
unsigned long lastTouchTime = 0;

void get_touch() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTouchTime < touchCooldown) {
        return; // Skip touch processing if within cooldown period
    }

    if (chsc6x_is_pressed()) {
        chsc6x_get_xy(&touchX, &touchY); // Get both X and Y coordinates

        if (touchStartTime == 0) {
            touchStartTime = millis();
        }
        unsigned long currentTapTime = millis();
        if (settingTime) {
            // Handle touch for time setting screen
            if (touchX < 50) {
                updateSettingValue(false); // Left arrow
            } else if (touchX > 190) {
                updateSettingValue(true); // Right arrow
            } else if (touchY > 200) {
                settingStep++;
                if (settingStep > 5) {
                    settingTime = false;
                    rtc.setDate(&currentDate);
                    rtc.setTime(&currentTime);
                }
            }
        } else {
            wasTouched = true;

            // Check for long press
            if (currentTapTime - touchStartTime >= longPressThreshold) {
                settingTime = true;
                touchStartTime = 0; // Reset touch start time
            } else if (currentTapTime - touchStartTime < longPressThreshold) {
                // Toggle is_happy on touch if not a long press
                is_happy = !is_happy;
            }
        }
        lastTouchTime = currentMillis; // Update last touch time
    } else {
        wasTouched = false;
        touchStartTime = 0; // Reset touch start time when touch is released
    }
}

void loop() {
    get_touch();
    if (settingTime) {
        displayTimeSettingScreen();
    } else {
        sprite.createSprite(240, 240);
        if (showTime) {
            displayDateTime();
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
}

void displayDateTime() {
    I2C_BM8563_TimeTypeDef time;
    I2C_BM8563_DateTypeDef date;
    rtc.getTime(&time);
    rtc.getDate(&date);

    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", time.hours, time.minutes, time.seconds);

    char dateStr[11];
    sprintf(dateStr, "%02d/%02d/%04d", date.month, date.date, date.year);

    sprite.fillSprite(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(3);
    sprite.drawString(timeStr, 30, 80);
    sprite.drawString(dateStr, 30, 140);
    sprite.pushSprite(0, 0);
}

void init_rtc() {
    Wire.begin();
    rtc.begin();

    rtc.getTime(&currentTime);
    rtc.getDate(&currentDate);

    // Check if the RTC is running by verifying if the time is valid
    if (currentTime.hours == 0 && currentTime.minutes == 0 && currentTime.seconds == 0) {
        I2C_BM8563_DateTypeDef date = {01, 01, 24}; // MM DD YY
        I2C_BM8563_TimeTypeDef time = {00, 00, 00}; // HH MM SS
        rtc.setDate(&date);
        rtc.setTime(&time);
    }
}