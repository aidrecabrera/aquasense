/**
 * * NOTES AND REFERENCES
 * * {1} Baud for serial communication.
 * * {2} Read the voltage on A0.
 * * {3} Value <-> voltage conversion
 * * {4-6} Log to serial monitor.
 **/

void setup()
{
    Serial.begin(9600); // * {1}
}

void loop()
{
    int sensorValue = analogRead(A0);             // * {2}
    float voltage = sensorValue * (5.0 / 1023.0); // * {3}

    Serial.print("Voltage on A0: "); // * {4}
    Serial.print(voltage);           // * {5}
    Serial.println(" V");            // * {6}

    delay(1000);
}