#include <ph4502c_sensor.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Constants
#define PH4502C_PH_LEVEL_PIN A0
#define PH4502C_TEMP_PIN A1
#define PUMP_PIN 8

#define PH_SAFE_LOW 7.0
#define PH_SAFE_HIGH 8.0
#define PH_TRIGGER_LOW 5.5
#define PH_TRIGGER_HIGH 8.5

#define PUMP_ON_TIME 30000
#define DELAY_TIME 1000

#define AVERAGE_INTERVAL 30000
#define READ_INTERVAL 1000

// Custom characters
byte waterDrop[] = {
    B00100,
    B00100,
    B01110,
    B11111,
    B11111,
    B01110,
    B00100,
    B00000};

byte thermometer[] = {
    B00100,
    B01010,
    B01010,
    B01010,
    B01010,
    B11111,
    B11111,
    B00000};

byte pump[] = {
    B01110,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111,
    B00000};

byte circle[] = {
    B00000,
    B01110,
    B10001,
    B10001,
    B10001,
    B10001,
    B01110,
    B00000};

byte celcius[] = {
    B00110,
    B01001,
    B01001,
    B00110,
    B00000,
    B00000,
    B00000,
    B00000};

byte signal1[] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00111,
    B00111};

byte signal2[] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00111,
    B00111,
    B00111,
    B00111};

byte signal3[] = {
    B00000,
    B00000,
    B00111,
    B00111,
    B00111,
    B00111,
    B00111,
    B00111};

byte signal4[] = {
    B00111,
    B00111,
    B00111,
    B00111,
    B00111,
    B00111,
    B00111,
    B00111};

// SMSNotifier class
class SMSNotifier
{
public:
    SMSNotifier(SoftwareSerial &sim800l, const String &phoneNumber)
        : sim800l(sim800l), phoneNumber(phoneNumber) {}

    String readSMS()
    {
        sim800l.println("AT+CMGR=1");
        delay(1000);
        String response = sim800l.readString();
        if (response.indexOf("+CMGR:") != -1)
        {
            int startIndex = response.indexOf("\r\n") + 2;
            int endIndex = response.indexOf("\r\n", startIndex);
            String message = response.substring(startIndex, endIndex);
            sim800l.println("AT+CMGD=1,4"); // Delete the SMS message
            return message;
        }
        return "";
    }

    void sendSMS(const String &message)
    {
        if (!smsSendingInProgress)
        {
            Serial.println("Sending Message");
            sim800l.println("AT+CMGF=1"); // Text Mode
            delay(200);
            Serial.println("Set SMS Number");
            sim800l.println("AT+CMGS=\"" + phoneNumber + "\"\r");
            delay(200);
            String SMS = message + " \n\n- Sent from AquaSense System.";
            sim800l.println(SMS);
            delay(100);
            sim800l.println((char)26); // ASCII code of CTRL+Z
            smsSendingInProgress = true;
            smsSendingStartTime = millis();
        }
    }

    void update()
    {
        if (smsSendingInProgress && (millis() - smsSendingStartTime >= SMS_SENDING_TIMEOUT))
        {
            smsSendingInProgress = false;
            _buffer = _readSerial();
        }
    }

private:
    SoftwareSerial &sim800l;
    String phoneNumber;
    int _timeout;
    String _buffer;
    bool smsSendingInProgress = false;
    unsigned long smsSendingStartTime = 0;
    const unsigned long SMS_SENDING_TIMEOUT = 5000; // Adjust the timeout as needed

    String _readSerial()
    {
        _timeout = 0;
        while (!sim800l.available() && _timeout < 12000)
        {
            delay(13);
            _timeout++;
        }
        if (sim800l.available())
        {
            return sim800l.readString();
        }
    }
};

// AquaSenseSystem class
class AquaSenseSystem
{
public:
    AquaSenseSystem()
        : lcd(0x27, 20, 4),
          ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN),
          sim800l(10, 11), // RX, TX pins for SIM800L
          smsNotifier(sim800l, "+639604377530")
    {
    }

    void setup()
    {
        sim800l.begin(9600);
        Serial.begin(9600);
        Serial.println("PH4502C Sensor...");
        pinMode(PUMP_PIN, OUTPUT);
        digitalWrite(PUMP_PIN, LOW);

        lcd.init();
        lcd.backlight();

        initializeCustomCharacters();

        // Bootup animation
        displayBootupAnimation();

        smsNotifier.sendSMS("AquaSense: System is online.");

        lcd.setCursor(14, 1);
        lcd.write(0);
        delay(1000);
        lcd.clear();
    }

    void loop()
    {
        readAndDisplaySensorData();
        checkAndUpdateAverage();
        processIncomingSMS();
        delay(DELAY_TIME);
    }

private:
    LiquidCrystal_I2C lcd;
    PH4502C_Sensor ph4502;
    SoftwareSerial sim800l;
    SMSNotifier smsNotifier;

    unsigned long pumpStartTime = 0;
    bool pumpIsOn = false;

    unsigned long lastAverageTime = 0;
    unsigned long lastReadTime = 0;

    float totalPhLevel = 0;
    float totalTemperature = 0;
    int numReadings = 0;

    float calibration = 3.27;

    void initializeCustomCharacters()
    {
        lcd.createChar(0, waterDrop);
        lcd.createChar(1, thermometer);
        lcd.createChar(2, pump);
        lcd.createChar(3, circle);
        lcd.createChar(4, celcius);
        lcd.createChar(5, signal1);
        lcd.createChar(6, signal2);
        lcd.createChar(7, signal3);
    }

    void displayBootupAnimation()
    {
        lcd.setCursor(4, 1);
        lcd.print("AquaSense");

        // Animation
        for (int i = 0; i < 3; i++)
        {
            lcd.setCursor(14, 1);
            lcd.write(0);
            delay(500);
            lcd.setCursor(14, 1);
            lcd.print(" ");
            delay(500);
        }
    }

    int getSignalStrength()
    {
        sim800l.println("AT+CSQ");
        delay(100);
        String response = sim800l.readString();
        int startIndex = response.indexOf(": ") + 2;
        int endIndex = response.indexOf(", ");
        String signalStrengthStr = response.substring(startIndex, endIndex);
        int signalStrengthValue = signalStrengthStr.toInt();
        return signalStrengthValue;
    }

    void displaySignalStrength(int signalStrengthValue)
    {
        if (signalStrengthValue == 0)
        {
            lcd.setCursor(14, 0);
            lcd.print("NOSIG");
        }
        else
        {
            int numBars = map(signalStrengthValue, 0, 31, 0, 5);
            lcd.setCursor(14, 0);
            for (int i = 0; i < 5; i++)
            {
                if (i < numBars)
                {
                    lcd.print(" ");
                }
            }
            for (int i = 0; i < 4; i++)
            {
                if (i < numBars)
                {
                    lcd.write(i + 5); // Use signal indicators 1-5
                }
                else
                {
                    lcd.print(" ");
                }
            }
        }
    }

    void processIncomingSMS()
    {
        String message = smsNotifier.readSMS();
        if (message != "")
        {
            Serial.println("Received SMS: " + message);
            if (message == "PUMP ON")
            {
                if (!pumpIsOn)
                {
                    digitalWrite(PUMP_PIN, LOW);
                    pumpIsOn = true;
                    smsNotifier.sendSMS("Pump has been turned on manually.");
                }
            }
            else if (message == "PUMP OFF")
            {
                if (pumpIsOn)
                {
                    digitalWrite(PUMP_PIN, HIGH);
                    pumpIsOn = false;
                    smsNotifier.sendSMS("Pump has been turned off manually.");
                }
            }
            else if (message == "COMMANDS")
            {
                String commandList = "Available commands:\n";
                commandList += "PUMP ON - Turn on the pump manually\n";
                commandList += "PUMP OFF - Turn off the pump manually\n";
                commandList += "STATUS - Get the current status of the system\n";
                commandList += "COMMANDS - List the available commands";
                smsNotifier.sendSMS(commandList);
            }
            else if (message == "STATUS")
            {
                String statusMessage = "Current system status:\n";
                statusMessage += "pH Level: " + String(readPHLevel(), 1) + "\n";
                statusMessage += "Temperature: " + String(readTemperature(), 1) + " °C\n";
                statusMessage += "Pump Status: " + String((pumpIsOn ? "ON" : "OFF"));
                smsNotifier.sendSMS(statusMessage);
            }
        }
    }

    void readAndDisplaySensorData()
    {
        int signalStrengthValue = getSignalStrength();
        displaySignalStrength(signalStrengthValue);

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
            String doDescription = getDissolvedOxygenDescription(doLevel);

            displaySensorData(phLevel, temperature, doDescription);
            displayCountdown();
            lastReadTime = millis();
        }
    }

    String getDissolvedOxygenDescription(float doLevel)
    {
        if (doLevel >= 8.0)
        {
            return "Optimal";
        }
        else if (doLevel >= 6.0)
        {
            return "High";
        }
        else if (doLevel >= 4.0)
        {
            return "Moderate";
        }
        else if (doLevel >= 2.0)
        {
            return "Low";
        }
        else if (doLevel >= 1.0)
        {
            return "Critical";
        }
        else if (doLevel >= 0.5)
        {
            return "Anoxic";
        }
        else if (doLevel >= 0.2)
        {
            return "Fluctuating";
        }
        else if (doLevel >= 0.1)
        {
            return "Saturated";
        }
        else
        {
            return "Supersaturated";
        }
    }

    void displaySensorData(float phLevel, float temperature, const String &doDescription)
    {
        lcd.setCursor(0, 0);
        lcd.print("DO: " + doDescription);
        lcd.setCursor(0, 1);
        lcd.print("pH: ");
        lcd.write(0);
        lcd.print(String(phLevel, 1));
        lcd.setCursor(0, 2);
        lcd.print("Temp: ");
        lcd.write(1);
        lcd.print(String(temperature, 1) + (char)4 + "C");
        if (pumpIsOn)
        {
            lcd.setCursor(0, 3);
            lcd.write(2);
            lcd.print(" Pump ON ");
        }
        else
        {
            lcd.setCursor(0, 3);
            lcd.write(2);
            lcd.print(" Pump OFF");
        }
    }

    void displayCountdown()
    {
        unsigned long remainingTime = AVERAGE_INTERVAL - (millis() - lastAverageTime);
        int seconds = max(0, (int)(remainingTime / 1000));
        String countdownStr = String(seconds);
        lcd.setCursor(15, 1);
        lcd.print("Next");
        lcd.setCursor(15, 2);
        if (seconds < 10)
        {
            lcd.print("0" + countdownStr + "sec");
        }
        else
        {
            lcd.print(countdownStr + "sec");
        }
    }

    void checkAndUpdateAverage()
    {
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
    }

    float readPHLevel()
    {
        float phLevel = (ph4502.read_ph_level() - calibration); // TODO: Adjust the ph offset
        return phLevel;
    }

    float readTemperature()
    {
        float temperature = ph4502.read_temp() / 27;
        return temperature;
    }

    void handlePump(float phLevelAvg)
    {
        int phLevel = phLevelAvg;
        Serial.println("AVG pH Level: " + String(phLevel));
        if (phLevel < PH_TRIGGER_LOW || phLevel > PH_TRIGGER_HIGH)
        {
            if (!pumpIsOn)
            {
                notifyPumpOn(phLevel);
                digitalWrite(PUMP_PIN, LOW);
                pumpStartTime = millis();
                pumpIsOn = true;
                Serial.println("pH level is outside the safe range. Turning on the pump.");
            }
        }
        else
        {
            if (pumpIsOn && (millis() - pumpStartTime >= PUMP_ON_TIME))
            {
                notifyPumpOff(phLevel);
                digitalWrite(PUMP_PIN, HIGH);
                pumpIsOn = false;
                Serial.println("pH level is within the safe range. Turning off the pump.");
            }
        }
    }

    void notifyPumpOn(float AvgPhLevel)
    {
        String message = "The pump has been turned on as the pH level is outside the safe range.";
        smsNotifier.sendSMS(message);
    }

    void notifyPumpOff(float AvgPhLevel)
    {
        String message = "The pump has been turned off as the pH level is within the safe range.";
        smsNotifier.sendSMS(message);
    }
};

// Main program
AquaSenseSystem aquaSenseSystem;

void setup()
{
    aquaSenseSystem.setup();
}

void loop()
{
    aquaSenseSystem.loop();
}