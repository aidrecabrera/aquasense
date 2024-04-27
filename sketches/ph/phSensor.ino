/**
 * * NOTES AND REFERENCES
 * * {1} pH level pin and temperature pin
 * * {2} PH4502C_Sensor object
 * * {3} Start serial communication
 * * {4} Print a message to the serial monitor
 * * {5} Initialize the sensor
 *
 * * {6} Read the temperature - Single, immediate temperature reading.
 * * {7} Read the pH level - Averaged pH level reading
 * * {8} Read the pH level single - Single, direct pH reading.
 */

#include <ph4502c_sensor.h>

// * {1}
#define PH4502C_PH_LEVEL_PIN A0
#define PH4502C_TEMP_PIN A1

PH4502C_Sensor ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN); // * {2}

void setup()
{
    Serial.begin(9600);                  // * {3}
    Serial.println("PH4502C Sensor..."); // * {4}
    ph4502.init();                       // * {5}
}

void loop()
{
    Serial.println("Temperature reading:" + String(ph4502.read_temp()));                 // * {6}
    Serial.println("pH Level Reading: " + String(ph4502.read_ph_level()));               // * {7}
    Serial.println("pH Level Single Reading: " + String(ph4502.read_ph_level_single())); // * {8}
    delay(1000);
}