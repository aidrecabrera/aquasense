#include <Arduino.h>

int pHSense = A0;
int samples = 10;
int switch_pump = 8;

float adc_resolution = 1024.0;

void setup()
{
    Serial.begin(9600);
    delay(100);
    Serial.println("cimpleo pH Sense");
    pinMode(switch_pump, OUTPUT);
}

float ph(float voltage)
{
    return 7 + ((2.5 - voltage) / 0.18);
}

void loop()
{
    int measurings = 0;
    for (int i = 0; i < samples; i++)
    {
        measurings += analogRead(pHSense);
        delay(10);
    }

    float voltage = 5.0 / adc_resolution * measurings / samples;
    float pH_level = ph(voltage);

    Serial.print("pH= ");
    Serial.println(pH_level);

    if (pH_level < 6.5 || pH_level > 7.5)
    {
        digitalWrite(switch_pump, HIGH);
        Serial.println("Pump is ON");

        while (pH_level < 6.5 || pH_level > 7.5)
        {
            delay(3000);

            measurings = 0;
            for (int i = 0; i < samples; i++)
            {
                measurings += analogRead(pHSense);
                delay(10);
            }

            voltage = 5.0 / adc_resolution * measurings / samples;
            pH_level = ph(voltage);

            Serial.print("pH= ");
            Serial.println(pH_level);
        }

        digitalWrite(switch_pump, LOW);
        Serial.println("Pump is OFF");
    }
    else
    {
        digitalWrite(switch_pump, LOW);
        Serial.println("Pump is OFF");
    }

    delay(3000);
}