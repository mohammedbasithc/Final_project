#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// BASE STATION
// ESP32 DevKit V1
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// ---------------- UART ----------------
#define UART_RX 16
#define UART_TX 17

HardwareSerial BaseSerial(2);

// ---------------- OUTPUTS ----------------
#define BUZZER_PIN 27
#define EMERGENCY_LED 25

// ---------------- OPTIONAL TEST BUTTON ----------------
#define TEST_BUTTON 4

bool lastButtonState = HIGH;

// =====================================================
// OLED
// =====================================================

void showOLED(
    String line1,
    String line2 = "",
    String line3 = "",
    String line4 = ""
)
{
    display.clearDisplay();

    display.setTextColor(
        SSD1306_WHITE
    );

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println(line1);

    if (line2.length() > 0)
    {
        display.setCursor(0, 16);
        display.println(line2);
    }

    if (line3.length() > 0)
    {
        display.setCursor(0, 32);
        display.println(line3);
    }

    if (line4.length() > 0)
    {
        display.setCursor(0, 48);
        display.println(line4);
    }

    display.display();
}

// =====================================================
// EMERGENCY
// =====================================================

void emergencyAlert(
    float acceleration,
    float latitude,
    float longitude
)
{
    Serial.println();
    Serial.println(
        "=============================="
    );

    Serial.println(
        "     EMERGENCY ALERT"
    );

    Serial.println(
        "=============================="
    );

    Serial.print(
        "Acceleration: "
    );

    Serial.print(
        acceleration,
        2
    );

    Serial.println(" G");

    Serial.print(
        "Latitude: "
    );

    Serial.println(
        latitude,
        6
    );

    Serial.print(
        "Longitude: "
    );

    Serial.println(
        longitude,
        6
    );

    digitalWrite(
        EMERGENCY_LED,
        HIGH
    );

    showOLED(
        "!!! EMERGENCY !!!",
        "ACCIDENT DETECTED",
        "G: " + String(
            acceleration,
            2
        ),
        String(latitude, 4)
    );

    tone(
        BUZZER_PIN,
        2000
    );

    delay(3000);

    noTone(
        BUZZER_PIN
    );

    showOLED(
        "BASE STATION",
        "Emergency received",
        "Location:",
        String(longitude, 4)
    );

    delay(3000);

    digitalWrite(
        EMERGENCY_LED,
        LOW
    );

    showOLED(
        "BASE STATION",
        "SYSTEM ONLINE",
        "Waiting for vehicle"
    );
}

// =====================================================
// PROCESS MESSAGE
// =====================================================

void processMessage(
    String message
)
{
    message.trim();

    if (message.length() == 0)
        return;

    Serial.print(
        "RX: "
    );

    Serial.println(
        message
    );

    // ---------------------------------------------
    // HEARTBEAT
    // ---------------------------------------------

    if (
        message ==
        "HEARTBEAT|VEHICLE_OK"
    )
    {
        showOLED(
            "BASE STATION",
            "VEHICLE ONLINE",
            "Heartbeat received",
            "STATUS: OK"
        );

        return;
    }

    // ---------------------------------------------
    // ACCIDENT
    // ---------------------------------------------

    if (
        message.startsWith(
            "ACCIDENT|"
        )
    )
    {
        float acceleration = 0;
        float latitude = 0;
        float longitude = 0;

        int p1 =
            message.indexOf('|');

        int p2 =
            message.indexOf(
                '|',
                p1 + 1
            );

        int p3 =
            message.indexOf(
                '|',
                p2 + 1
            );

        if (
            p1 > 0 &&
            p2 > p1 &&
            p3 > p2
        )
        {
            acceleration =
                message.substring(
                    p1 + 1,
                    p2
                ).toFloat();

            latitude =
                message.substring(
                    p2 + 1,
                    p3
                ).toFloat();

            longitude =
                message.substring(
                    p3 + 1
                ).toFloat();

            emergencyAlert(
                acceleration,
                latitude,
                longitude
            );
        }

        return;
    }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(
        115200
    );

    delay(1000);

    Serial.println();
    Serial.println(
        "=============================="
    );

    Serial.println(
        " BASE STATION"
    );

    Serial.println(
        " ESP32 STARTING"
    );

    Serial.println(
        "=============================="
    );

    pinMode(
        BUZZER_PIN,
        OUTPUT
    );

    pinMode(
        EMERGENCY_LED,
        OUTPUT
    );

    pinMode(
        TEST_BUTTON,
        INPUT_PULLUP
    );

    digitalWrite(
        EMERGENCY_LED,
        LOW
    );

    // I2C
    Wire.begin(
        21,
        22
    );

    // OLED
    Serial.println(
        "Initializing BASE OLED..."
    );

    if (
        !display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDR
        )
    )
    {
        Serial.println(
            "BASE OLED: FAILED"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(
        "BASE OLED: OK"
    );

    showOLED(
        "BASE STATION",
        "Starting..."
    );

    delay(1000);

    // UART2
    BaseSerial.begin(
        115200,
        SERIAL_8N1,
        UART_RX,
        UART_TX
    );

    Serial.println(
        "UART2: READY"
    );

    showOLED(
        "BASE STATION",
        "SYSTEM ONLINE",
        "Waiting for vehicle",
        "UART READY"
    );

    Serial.println();
    Serial.println(
        "BASE STATION READY"
    );
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    // Receive UART
    if (BaseSerial.available())
    {
        String message =
            BaseSerial.readStringUntil(
                '\n'
            );

        processMessage(
            message
        );
    }

    // Local test button
    bool buttonState =
        digitalRead(
            TEST_BUTTON
        );

    if (
        buttonState == LOW &&
        lastButtonState == HIGH
    )
    {
        Serial.println(
            "LOCAL EMERGENCY TEST"
        );

        emergencyAlert(
            5.20,
            11.016800,
            76.955800
        );

        delay(500);
    }

    lastButtonState =
        buttonState;

    delay(10);
}