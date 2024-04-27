#include <ph4502c_sensor.h>
#include <LiquidCrystal_I2C.h>

#define PH4502C_PH_LEVEL_PIN A0
#define PH4502C_TEMP_PIN A1
#define PUMP_PIN 8

#define PH_SAFE_LOW 7.0
#define PH_SAFE_HIGH 8.0
#define PH_TRIGGER_LOW 5.5
#define PH_TRIGGER_HIGH 8.5

#define PUMP_ON_TIME 30000
#define DELAY_TIME 100

#define AVERAGE_INTERVAL 30000
#define READ_INTERVAL 100

LiquidCrystal_I2C lcd(0x27, 20, 4);
PH4502C_Sensor ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN);

unsigned long pumpStartTime = 0;
bool pumpIsOn = false;

unsigned long lastAverageTime = 0;
unsigned long lastReadTime = 0;

float totalPhLevel = 0;
float totalTemperature = 0;
int numReadings = 0;

float calibration = 24.58;
int buf[10], temp;

void setup()
{
    Serial.begin(9600);
    Serial.println("PH4502C Sensor...");
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(4, 0);
    lcd.print("AquaSense!");
}

float readPHLevel()
{
    unsigned long int avgValue = 0;

    for (int i = 0; i < 10; i++)
    {
        buf[i] = analogRead(PH4502C_PH_LEVEL_PIN);
        delay(30);
    }

    for (int i = 0; i < 9; i++)
    {
        for (int j = i + 1; j < 10; j++)
        {
            if (buf[i] > buf[j])
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    for (int i = 2; i < 8; i++)
        avgValue += buf[i];

    float pHVol = (float)avgValue * 5.0 / 1024 / 6;
    float phValue = -5.70 * pHVol + calibration;

    return phValue;
}

float readTemperature()
{
    float temperature = ph4502.read_temp() / 27;
    return temperature;
}

void readAndPrintSensorData()
{
    if (millis() - lastReadTime >= READ_INTERVAL)
    {
        float phLevel = readPHLevel();
        float temperature = readTemperature();

        totalPhLevel += phLevel;
        totalTemperature += temperature;
        numReadings++;

        Serial.println("Current Temperature: " + String(temperature) + " °C");
        Serial.println("Current pH Level: " + String(phLevel));

        float doLevel = 14.6 - 0.4 * phLevel; // Inversely proportional to pH level
        String doDescription;

        if (doLevel >= 8.0)
        {
            doDescription = "Optimal";
        }
        else if (doLevel >= 6.0)
        {
            doDescription = "High";
        }
        else if (doLevel >= 4.0)
        {
            doDescription = "Moderate";
        }
        else if (doLevel >= 2.0)
        {
            doDescription = "Low";
        }
        else if (doLevel >= 1.0)
        {
            doDescription = "Critical";
        }
        else if (doLevel >= 0.5)
        {
            doDescription = "Anoxic";
        }
        else if (doLevel >= 0.2)
        {
            doDescription = "Fluctuating";
        }
        else if (doLevel >= 0.1)
        {
            doDescription = "Saturated";
        }
        else
        {
            doDescription = "Supersaturated";
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("DO: " + doDescription);
        lcd.setCursor(0, 1);
        lcd.print("pH: " + String(phLevel, 2));
        lcd.setCursor(0, 2);
        lcd.print("Temp: " + String(temperature, 1) + " C");

        lastReadTime = millis();
    }
}

void handlePump(float phLevel)
{
    Serial.println("AVG pH Level: " + String(phLevel));
    if (phLevel < PH_TRIGGER_LOW || phLevel > PH_TRIGGER_HIGH)
    {
        if (!pumpIsOn)
        {
            digitalWrite(PUMP_PIN, LOW);
            pumpStartTime = millis();
            pumpIsOn = true;
            Serial.println("pH level is outside the safe range. Turning on the pump.");
            lcd.setCursor(0, 3);
            lcd.print("Pump ON for " + String(PUMP_ON_TIME / 1000) + "s");
        }
    }
    else
    {
        if (pumpIsOn || millis() - pumpStartTime >= PUMP_ON_TIME)
        {
            digitalWrite(PUMP_PIN, HIGH);
            pumpIsOn = false;
            Serial.println("pH level is within the safe range. Turning off the pump.");
            lcd.setCursor(0, 3);
            lcd.print("Pump OFF ");
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
        Serial.println("Time until next pH level check: " + String(remainingTime / 1000) + "s");
    }
    delay(DELAY_TIME);
}