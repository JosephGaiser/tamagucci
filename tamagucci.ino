#include <TFT_eSPI.h>
#include <SPI.h>
#include <I2C_BM8563.h>
#define USE_TFT_ESPI_LIBRARY
#include "lv_xiao_round_screen.h"
#include "face_smile.cpp"
#include "face_smile_b.cpp"
#include "face_frown.cpp"
#include "face_frown_b.cpp"
#include "tImage.h"
#include <Adafruit_NeoPixel.h>

int Power = 11;
int PIN  = 12;
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
TFT_eSprite sprite = TFT_eSprite(&tft);

I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);
const int chipSelect = D2;

bool is_happy = true; // Flag to track the current image
int blinkCounter = 0; // Counter to manage blinking
int blinkInterval = 50; // Interval for blinking
int randomBlinkInterval = blinkInterval; // Randomized blink interval
int blinkDuration = 2; // Duration for which the blink image is displayed
unsigned long previousMillis = 0; // Store the last time the brightness was updated
const long interval = 20; // Interval at which to update the brightness (milliseconds)


void setup() {
    pixels.begin();
    pinMode(Power, OUTPUT);
    digitalWrite(Power, HIGH);

    tft.init();
    tft.setRotation(0);
    randomSeed(analogRead(0)); // Initialize random seed
    init_rtc(); // Initialize the RTC
}


void addNoise(uint16_t* imageData, int width, int height, int noiseLevel) {
    for (int i = 0; i < width * height; i++) {
        if (random(0, 100) < noiseLevel) {
            imageData[i] = imageData[i] ^ 0xFFFF; // Invert the color to add noise
        }
    }
}

void loop() {
    get_touch();
    sprite.createSprite(240, 240);
    if (is_happy) {
        if (blinkCounter < randomBlinkInterval || blinkCounter >= randomBlinkInterval + blinkDuration) {
            addNoise((uint16_t*)face_smile.data, face_smile.width, face_smile.height, 5); // Add noise to the normal smile image
            sprite.pushImage(0, 0, face_smile.width, face_smile.height, (uint16_t*)face_smile.data);
        } else {
            addNoise((uint16_t*)face_smile_b.data, face_smile_b.width, face_smile_b.height, 5); // Add noise to the blinking smile image
            sprite.pushImage(0, 0, face_smile_b.width, face_smile_b.height, (uint16_t*)face_smile_b.data);
        }
    } else {
        if (blinkCounter < randomBlinkInterval || blinkCounter >= randomBlinkInterval + blinkDuration) {
            addNoise((uint16_t*)face_frown.data, face_frown.width, face_frown.height, 5); // Add noise to the normal frown image
            sprite.pushImage(0, 0, face_frown.width, face_frown.height, (uint16_t*)face_frown.data);
        } else {
            addNoise((uint16_t*)face_frown_b.data, face_frown_b.width, face_frown_b.height, 5); // Add noise to the blinking frown image
            sprite.pushImage(0, 0, face_frown_b.width, face_frown_b.height, (uint16_t*)face_frown_b.data);
        }
    }
    sprite.pushSprite(0, 0);  // Placement of the drawn sprite
    sprite.deleteSprite();

    // Update blink counter
    blinkCounter++;
    if (blinkCounter >= randomBlinkInterval + blinkDuration) {
        blinkCounter = 0; // Reset the counter after a full blink cycle
        randomBlinkInterval = blinkInterval + random(0, 20); // Add randomness to the blink interval
    }

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Calculate the brightness using a sine wave function
        float brightness = (sin(currentMillis / 1000.0 * 2 * PI) + 1) / 2 * 255;

        if (is_happy) {
            pixels.setPixelColor(0, pixels.Color(0, brightness, 0)); // Green for happy
        } else {
            pixels.setPixelColor(0, pixels.Color(brightness, 0, 0)); // Red for sad
        }
        pixels.show();
    }
}

bool wasTouched = false; // Flag to track the previous touch state

void get_touch() {
    lv_coord_t touchX, touchY;

    if (chsc6x_is_pressed()) {
        if (!wasTouched) { // Only toggle if it was not previously touched
            chsc6x_get_xy(&touchX, &touchY);
            if (touchX > 240 || touchY > 240) {
                touchX = 0;
                touchY = 0;
            }

            // Toggle the image
            is_happy = !is_happy;
            wasTouched = true; // Update the touch state
        }
    } else {
        wasTouched = false; // Reset the touch state when not pressed
    }
}

void init_rtc() {
    Wire.begin();
    rtc.begin();
    I2C_BM8563_DateTypeDef date;
    date.month = 11;
    date.date = 7;
    date.year = 2024;
    I2C_BM8563_TimeTypeDef time = {18, 30, 20};
    rtc.setDate(&date);
    rtc.setTime(&time);
}