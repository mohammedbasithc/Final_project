#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define ACCIDENT_BUTTON 4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_MPU6050 mpu;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

bool accidentDetected = false;


// ================= OLED: NORMAL =================

void showNormal(float acceleration)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("VEHICLE BLACK BOX");

    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(18, 20);
    display.println("NORMAL");

    display.setTextSize(1);
    display.setCursor(5, 50);
    display.print("ACC: ");
    display.print(acceleration, 1);
    display.print(" m/s2");

    display.display();
}


// ================= OLED: ACCIDENT =================

void showAccident()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(4, 3);
    display.println("ACCIDENT!");

    display.drawLine(0, 23, 127, 23, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(18, 32);
    display.println("EMERGENCY ALERT");

    display.setCursor(24, 47);
    display.println("EVENT DETECTED");

    display.display();
}


// ================= SETUP =================

void setup()
{
    Serial.begin(115200);

    pinMode(ACCIDENT_BUTTON, INPUT_PULLUP);

    // I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    // MPU6050
    if (!mpu.begin())
    {
        Serial.println("ERROR: MPU6050 NOT FOUND!");

        while (1)
        {
            delay(100);
        }
    }

    Serial.println("MPU6050 connected.");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);


    // OLED
    if (!display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDRESS))
    {
        Serial.println("ERROR: OLED NOT FOUND!");

        while (1)
        {
            delay(100);
        }
    }

    Serial.println("OLED connected.");
    Serial.println();
    Serial.println("==============================");
    Serial.println(" VEHICLE BLACK BOX SIMULATION");
    Serial.println("==============================");

    showNormal(0.0);
}


// ================= LOOP =================

void loop()
{
    sensors_event_t acceleration;
    sensors_event_t gyro;
    sensors_event_t temperature;

    mpu.getEvent(
        &acceleration,
        &gyro,
        &temperature
    );


    // Acceleration
    float ax = acceleration.acceleration.x;
    float ay = acceleration.acceleration.y;
    float az = acceleration.acceleration.z;

    float totalAcceleration = sqrt(
        ax * ax +
        ay * ay +
        az * az
    );


    // Serial monitor
    Serial.print("AX: ");
    Serial.print(ax, 2);

    Serial.print(" | AY: ");
    Serial.print(ay, 2);

    Serial.print(" | AZ: ");
    Serial.print(az, 2);

    Serial.print(" | TOTAL: ");
    Serial.print(totalAcceleration, 2);

    Serial.println(" m/s2");


    // ------------------------------
    // Manual accident simulation
    // ------------------------------

    if (digitalRead(ACCIDENT_BUTTON) == LOW)
    {
        accidentDetected = true;
    }


    // ------------------------------
    // Automatic accident detection
    // ------------------------------

    if (totalAcceleration > 25.0)
    {
        accidentDetected = true;
    }


    // ------------------------------
    // Accident event
    // ------------------------------

    if (accidentDetected)
    {
        Serial.println();
        Serial.println("******************************");
        Serial.println("       ACCIDENT DETECTED");
        Serial.println("******************************");

        Serial.print("Acceleration = ");
        Serial.print(totalAcceleration, 2);
        Serial.println(" m/s2");

        Serial.println("Emergency alert triggered.");

        showAccident();

        delay(3000);

        accidentDetected = false;

        Serial.println("Vehicle returned to normal.");

        showNormal(totalAcceleration);
    }
    else
    {
        showNormal(totalAcceleration);
    }

    delay(200);
}