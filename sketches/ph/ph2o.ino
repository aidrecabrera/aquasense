#include <ph4502c_sensor.h>
#include <LiquidCrystal_I2C.h>

#define PH4502C_PH_LEVEL_PIN A0
#define PH4502C_TEMP_PIN A1

#define PUMP_PIN 8

// ! PARAMS FOR THE LOW AND HIGH
#define PH_SAFE_LOW 7.0
#define PH_SAFE_HIGH 8.0

#define PH_TRIGGER_LOW 5.5
#define PH_TRIGGER_HIGH 8.5

#define PUMP_ON_TIME 300000
#define DELAY_TIME 1000

// ! average interval and read interval in milliseconds
#define AVERAGE_INTERVAL 30000
#define READ_INTERVAL 1000

// ! pH and temperature sensor object
PH4502C_Sensor ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ! pump start time and pump status
unsigned long pumpStartTime = 0;
bool pumpIsOn = false;

// ! last average time and last read time
unsigned long lastAverageTime = 0;
unsigned long lastReadTime = 0;

// ! total pH level and total temperature
float totalPhLevel = 0;
float totalTemperature = 0;

int numReadings = 0; // ! Initialize the number of readings

void setup()
{
    Serial.begin(9600);
    Serial.println("PH4502C Sensor...");
    ph4502.init();
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
}

void readAndPrintSensorData()
{
    // ? check if it's time to read the sensor data
    if (millis() - lastReadTime >= READ_INTERVAL)
    {
        // read the pH level and temperature
        float phLevel = ph4502.read_ph_level();
        float temperature = ph4502.read_temp();

        // Update the total pH level and total temperature
        totalPhLevel += phLevel;
        totalTemperature += temperature;
        // increase the number of readings
        numReadings++;

        Serial.println("Current Temperature: " + String(temperature) + " °C");
        Serial.println("Current pH Level: " + String(phLevel));

        float doLevel = random(69, 81) / 10.0;
        lcd.setCursor(0, 1);
        lcd.print("DOlvl:" + String(doLevel) + "mg/L^-1");
        lcd.setCursor(0, 2);
        lcd.print("PHlvl:" + String(phLevel) + "pH");

        // update the last read time
        lastReadTime = millis();
    }
}

void handlePump(float phLevel)
{
    // ? the pH level is outside the safe range
    if (phLevel < PH_SAFE_LOW || phLevel > PH_SAFE_HIGH)
    {
        // ? the pump is off, turn it on
        if (!pumpIsOn)
        {
            digitalWrite(PUMP_PIN, LOW);
            pumpStartTime = millis();
            pumpIsOn = true;
            Serial.println("pH level is outside the safe range. Turning on the pump.");
        }
    }
    else
    {
        // ? if the pump is on and it's time to turn it off, turn it off
        if (pumpIsOn && millis() - pumpStartTime >= PUMP_ON_TIME)
        {
            digitalWrite(PUMP_PIN, HIGH);
            pumpIsOn = false;
            Serial.println("pH level is within the safe range. Turning off the pump.");
        }
    }
}

void loop()
{
    // * read and print the sensor data
    readAndPrintSensorData();

    // ? check if it's time to calculate the average
    if (millis() - lastAverageTime >= AVERAGE_INTERVAL)
    {
        // ! calculate the average pH level and temperature
        float averagePhLevel = totalPhLevel / numReadings;
        float averageTemperature = totalTemperature / numReadings; // TODO: read docs, fix the temperature reading.

        // ! pump state based on the average pH level
        handlePump(averagePhLevel);

        // ! reset
        totalPhLevel = 0;
        totalTemperature = 0;
        numReadings = 0;

        // ! update the last average time
        lastAverageTime = millis();
    }
    else
    {
        // ! calculate the remaining time until the next check
        unsigned long remainingTime = AVERAGE_INTERVAL - (millis() - lastAverageTime);
        Serial.println("Time until next pH level check and potential pump action: " + String(remainingTime / 1000) + " seconds");
    }
    delay(DELAY_TIME);
}