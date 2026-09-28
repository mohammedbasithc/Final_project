#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// =====================================================
// VEHICLE BLACK BOX
// ESP32 DevKit V1
// =====================================================

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// ---------------- MPU6050 ----------------
Adafruit_MPU6050 mpu;

// ---------------- SD CARD ----------------
#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

// ---------------- BUTTONS ----------------
#define ACCIDENT_BUTTON 4
#define LINK_BUTTON     33

// ---------------- OUTPUTS ----------------
#define BUZZER_PIN 27
#define GREEN_LED  25
#define RED_LED    26

// ---------------- UART ----------------
#define UART_RX 16
#define UART_TX 17

HardwareSerial VehicleSerial(2);

// ---------------- GPS SIMULATION ----------------
float latitude  = 11.016800;
float longitude = 76.955800;

// ---------------- SYSTEM VARIABLES ----------------
bool sdOK = false;
bool mpuOK = false;
bool communicationAvailable = true;

bool lastAccidentButton = HIGH;
bool lastLinkButton = HIGH;

unsigned long lastSensorTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastLogTime = 0;

const unsigned long SENSOR_INTERVAL = 1000;
const unsigned long HEARTBEAT_INTERVAL = 5000;
const unsigned long LOG_INTERVAL = 2000;

// =====================================================
// OLED FUNCTIONS
// =====================================================

void oledMessage(
    String line1,
    String line2 = "",
    String line3 = "",
    String line4 = ""
)
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
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
// SD CARD
// =====================================================

void initializeSD()
{
    Serial.println("Initializing SD card...");

    SPI.begin(
        SD_SCK,
        SD_MISO,
        SD_MOSI,
        SD_CS
    );

    if (!SD.begin(SD_CS, SPI))
    {
        Serial.println("SD CARD: FAILED");
        sdOK = false;
        return;
    }

    sdOK = true;

    Serial.println("SD CARD: OK");

    // Create CSV file if it does not exist
    if (!SD.exists("/blackbox.csv"))
    {
        File file = SD.open(
            "/blackbox.csv",
            FILE_WRITE
        );

        if (file)
        {
            file.println(
                "time_ms,acceleration_g,latitude,longitude,status"
            );

            file.close();

            Serial.println(
                "blackbox.csv created"
            );
        }
        else
        {
            Serial.println(
                "Could not create blackbox.csv"
            );
        }
    }
}

// =====================================================
// LOG DATA
// =====================================================

void logData(
    float accelerationG,
    const char *status
)
{
    if (!sdOK)
    {
        Serial.println("SD not available");
        return;
    }

    File file = SD.open(
        "/blackbox.csv",
        FILE_APPEND
    );

    if (!file)
    {
        Serial.println("SD WRITE: FAILED");
        return;
    }

    file.print(millis());
    file.print(",");

    file.print(accelerationG, 3);
    file.print(",");

    file.print(latitude, 6);
    file.print(",");

    file.print(longitude, 6);
    file.print(",");

    file.println(status);

    file.close();

    Serial.println("SD WRITE: OK");
}

// =====================================================
// READ ACCELERATION
// =====================================================

float getAccelerationG()
{
    if (!mpuOK)
        return 0.0;

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;

    mpu.getEvent(
        &accel,
        &gyro,
        &temp
    );

    float magnitude =
        sqrt(
            accel.acceleration.x *
            accel.acceleration.x +

            accel.acceleration.y *
            accel.acceleration.y +

            accel.acceleration.z *
            accel.acceleration.z
        );

    return magnitude / 9.80665;
}

// =====================================================
// SEND UART MESSAGE
// =====================================================

void sendHeartbeat()
{
    if (!communicationAvailable)
        return;

    VehicleSerial.println(
        "HEARTBEAT|VEHICLE_OK"
    );

    Serial.println(
        "UART -> HEARTBEAT"
    );
}

void sendAccident(
    float accelerationG
)
{
    if (!communicationAvailable)
    {
        Serial.println(
            "UART LINK FAILED - ACCIDENT NOT SENT"
        );
        return;
    }

    VehicleSerial.print("ACCIDENT|");
    VehicleSerial.print(accelerationG, 2);
    VehicleSerial.print("|");
    VehicleSerial.print(latitude, 6);
    VehicleSerial.print("|");
    VehicleSerial.println(longitude, 6);

    Serial.println(
        "UART -> ACCIDENT SENT"
    );
}

// =====================================================
// ACCIDENT HANDLER
// =====================================================

void handleAccident()
{
    float accelerationG =
        getAccelerationG();

    Serial.println();
    Serial.println("==============================");
    Serial.println("      ACCIDENT DETECTED");
    Serial.println("==============================");

    Serial.print("Acceleration: ");
    Serial.print(accelerationG, 2);
    Serial.println(" G");

    Serial.print("Latitude: ");
    Serial.println(latitude, 6);

    Serial.print("Longitude: ");
    Serial.println(longitude, 6);

    // LED
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    // Buzzer
    tone(BUZZER_PIN, 2000);

    oledMessage(
        "!!! ACCIDENT !!!",
        "Emergency detected",
        "Saving data...",
        "Sending alert..."
    );

    // Save accident to SD
    logData(
        accelerationG,
        "ACCIDENT"
    );

    // Send to base station
    sendAccident(
        accelerationG
    );

    delay(2000);

    noTone(BUZZER_PIN);

    oledMessage(
        "ACCIDENT RECORDED",
        sdOK ? "SD: SAVED" : "SD: FAILED",
        communicationAvailable
            ? "BASE: SENT"
            : "BASE: OFFLINE"
    );

    delay(2000);

    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);

    oledMessage(
        "VEHICLE BLACK BOX",
        "SYSTEM NORMAL",
        "Monitoring...",
        sdOK ? "SD: OK" : "SD: ERROR"
    );
}

// =====================================================
// LINK BUTTON
// =====================================================

void checkLinkButton()
{
    bool state =
        digitalRead(LINK_BUTTON);

    if (
        state == LOW &&
        lastLinkButton == HIGH
    )
    {
        communicationAvailable =
            !communicationAvailable;

        Serial.print(
            "Communication: "
        );

        Serial.println(
            communicationAvailable
                ? "ONLINE"
                : "OFFLINE"
        );

        if (communicationAvailable)
        {
            oledMessage(
                "COMMUNICATION",
                "LINK ONLINE",
                "Base connection OK"
            );

            digitalWrite(
                GREEN_LED,
                HIGH
            );
        }
        else
        {
            oledMessage(
                "COMMUNICATION",
                "LINK FAILED",
                "Base disconnected"
            );

            digitalWrite(
                GREEN_LED,
                LOW
            );
        }

        delay(500);
    }

    lastLinkButton = state;
}

// =====================================================
// ACCIDENT BUTTON
// =====================================================

void checkAccidentButton()
{
    bool state =
        digitalRead(ACCIDENT_BUTTON);

    if (
        state == LOW &&
        lastAccidentButton == HIGH
    )
    {
        handleAccident();

        delay(300);
    }

    lastAccidentButton = state;
}

// =====================================================
// SENSOR + LOGGING
// =====================================================

void processSensor()
{
    float accelerationG =
        getAccelerationG();

    Serial.print(
        "Acceleration = "
    );

    Serial.print(
        accelerationG,
        2
    );

    Serial.println(
        " G"
    );

    if (millis() - lastLogTime >= LOG_INTERVAL)
    {
        lastLogTime = millis();

        logData(
            accelerationG,
            "NORMAL"
        );
    }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println(" VEHICLE BLACK BOX");
    Serial.println(" ESP32 STARTING");
    Serial.println("==============================");

    // GPIO
    pinMode(
        ACCIDENT_BUTTON,
        INPUT_PULLUP
    );

    pinMode(
        LINK_BUTTON,
        INPUT_PULLUP
    );

    pinMode(
        BUZZER_PIN,
        OUTPUT
    );

    pinMode(
        GREEN_LED,
        OUTPUT
    );

    pinMode(
        RED_LED,
        OUTPUT
    );

    digitalWrite(
        GREEN_LED,
        HIGH
    );

    digitalWrite(
        RED_LED,
        LOW
    );

    // I2C
    Wire.begin(
        21,
        22
    );

    // OLED
    Serial.println(
        "Initializing vehicle OLED..."
    );

    if (
        !display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDR
        )
    )
    {
        Serial.println(
            "VEHICLE OLED: FAILED"
        );
    }
    else
    {
        Serial.println(
            "VEHICLE OLED: OK"
        );

        oledMessage(
            "VEHICLE BLACK BOX",
            "Starting system..."
        );
    }

    delay(1000);

    // MPU6050
    Serial.println(
        "Initializing MPU6050..."
    );

    if (mpu.begin())
    {
        mpuOK = true;

        Serial.println(
            "MPU6050: OK"
        );

        mpu.setAccelerometerRange(
            MPU6050_RANGE_8_G
        );

        mpu.setGyroRange(
            MPU6050_RANGE_500_DEG
        );
    }
    else
    {
        Serial.println(
            "MPU6050: FAILED"
        );

        mpuOK = false;
    }

    // SD
    initializeSD();

    // UART2
    VehicleSerial.begin(
        115200,
        SERIAL_8N1,
        UART_RX,
        UART_TX
    );

    Serial.println(
        "UART2: READY"
    );

    oledMessage(
        "VEHICLE BLACK BOX",
        "SYSTEM ONLINE",
        mpuOK ? "MPU: OK" : "MPU: ERROR",
        sdOK ? "SD: OK" : "SD: ERROR"
    );

    delay(2000);

    oledMessage(
        "SYSTEM NORMAL",
        "Monitoring vehicle",
        "ACCIDENT -> GPIO4",
        "LINK -> GPIO33"
    );

    Serial.println();
    Serial.println(
        "VEHICLE SYSTEM READY"
    );
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    checkAccidentButton();

    checkLinkButton();

    if (
        millis() - lastSensorTime
        >= SENSOR_INTERVAL
    )
    {
        lastSensorTime =
            millis();

        processSensor();
    }

    if (
        millis() - lastHeartbeatTime
        >= HEARTBEAT_INTERVAL
    )
    {
        lastHeartbeatTime =
            millis();

        sendHeartbeat();
    }

    delay(10);
}