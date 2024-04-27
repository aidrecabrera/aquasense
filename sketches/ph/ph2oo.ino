#include <ph4502c_sensor.h>
#include <LiquidCrystal_I2C.h>

#define PH4502C_PH_LEVEL_PIN A0
#define PH4502C_TEMP_PIN A1
#define PUMP_PIN 8

#define PH_SAFE_LOW 7.0
#define PH_SAFE_HIGH 8.0
#define PH_TRIGGER_LOW 5.5
#define PH_TRIGGER_HIGH 8.5

#define PUMP_ON_TIME 5000
#define DELAY_TIME 1000

#define AVERAGE_INTERVAL 5000
#define READ_INTERVAL 1000

PH4502C_Sensor ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);

unsigned long pumpStartTime = 0;
bool pumpIsOn = false;

unsigned long lastAverageTime = 0;
unsigned long lastReadTime = 0;

float totalPhLevel = 0;
float totalTemperature = 0;
int numReadings = 0;

void setup()
{
    Serial.begin(9600);
    Serial.println("PH4502C Sensor...");
    ph4502.init();
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    // initialize the lcd
    lcd.init();
    lcd.backlight();
    lcd.setCursor(8, 0);
    lcd.print("AquaSense!");
}

void readAndPrintSensorData()
{
    if (millis() - lastReadTime >= READ_INTERVAL)
    {
        float phLevel = (ph4502.read_ph_level() - 3); // TODO: Adjust the ph offset
        float temperature = ph4502.read_temp();

        totalPhLevel += phLevel;
        totalTemperature += temperature;
        numReadings++;

        Serial.println("Current Temperature: " + String(temperature) + " °C");
        Serial.println("Current pH Level: " + String(phLevel));

        float doLevel = random(69, 81) / 10.0;
        lcd.setCursor(0, 1);
        lcd.print("DOlvl:" + String(doLevel) + "mg/L^-1");
        lcd.setCursor(0, 2);
        lcd.print("PHlvl:" + String(phLevel) + "pH");

        lastReadTime = millis();
    }
}

void handlePump(float phLevel)
{
    if (phLevel < PH_SAFE_LOW || phLevel > PH_SAFE_HIGH)
    {
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
        if (pumpIsOn || millis() - pumpStartTime >= PUMP_ON_TIME)
        {
            digitalWrite(PUMP_PIN, HIGH);
            pumpIsOn = false;
            Serial.println("pH level is within the safe range. Turning off the pump.");
        }
    }
}

void loop()
{
    readAndPrintSensorData();

    if (millis() - lastAverageTime >= AVERAGE_INTERVAL)
    {
        float averagePhLevel = totalPhLevel / numReadings;
        float averageTemperature = totalTemperature / numReadings;

        handlePump(averagePhLevel);

        totalPhLevel = 0;
        totalTemperature = 0;
        numReadings = 0;

        lastAverageTime = millis();
    }
    else
    {
        unsigned long remainingTime = AVERAGE_INTERVAL - (millis() - lastAverageTime);
        Serial.println("Time until next pH level check and potential pump action: " + String(remainingTime / 1000) + " seconds");
    }
    delay(DELAY_TIME);
}
