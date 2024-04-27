#include <ph4502c_sensor.h>
#include <LiquidCrystal_I2C.h>

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

// Logos
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

// Function prototypes
void setupAquaSenseSystem();
void loopAquaSenseSystem();
void readAndPrintSensorData();
float readPHLevel();
float readTemperature();
void handlePump(float phLevelAvg);

// Global variables
LiquidCrystal_I2C lcd(0x27, 20, 4);
PH4502C_Sensor ph4502(PH4502C_PH_LEVEL_PIN, PH4502C_TEMP_PIN);

unsigned long pumpStartTime = 0;
bool pumpIsOn = true;

unsigned long lastAverageTime = 0;
unsigned long lastReadTime = 0;

float totalPhLevel = 0;
float totalTemperature = 0;
int numReadings = 0;

float calibration = 3.27;

void setup()
{
    setupAquaSenseSystem();
}

void loop()
{
    loopAquaSenseSystem();
}

void setupAquaSenseSystem()
{
    Serial.begin(9600);
    Serial.println("PH4502C Sensor...");
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);

    lcd.init();
    lcd.backlight();

    lcd.createChar(0, waterDrop);
    lcd.createChar(1, thermometer);
    lcd.createChar(2, pump);
    lcd.createChar(3, circle);
    lcd.createChar(4, celcius);
    lcd.createChar(5, signal1);
    lcd.createChar(6, signal2);
    lcd.createChar(7, signal3);

    // Bootup animation
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

    delay(1000);
    lcd.setCursor(14, 1);
    lcd.write(0);
    delay(1000);
    lcd.clear();
}

void loopAquaSenseSystem()
{
    readAndPrintSensorData();

    if (millis() - lastAverageTime >= AVERAGE_INTERVAL)
    {
        float averagePhLevel = totalPhLevel / numReadings;

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

void readAndPrintSensorData()
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
        doDescription = "Optimal";
    else if (doLevel >= 6.0)
        doDescription = "High";
    else if (doLevel >= 4.0)
        doDescription = "Moderate";
    else if (doLevel >= 2.0)
        doDescription = "Low";
    else if (doLevel >= 1.0)
        doDescription = "Critical";
    else if (doLevel >= 0.5)
        doDescription = "Anoxic";
    else if (doLevel >= 0.2)
        doDescription = "Fluctuating";
    else if (doLevel >= 0.1)
        doDescription = "Saturated";
    else
        doDescription = "Supersaturated";

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

    unsigned long remainingTime = AVERAGE_INTERVAL - (millis() - lastAverageTime);
    if (remainingTime < 0)
        remainingTime = 0;
    int seconds = remainingTime / 1000;
    String countdownStr = String(seconds);
    lcd.setCursor(15, 1);
    lcd.print("Next");
    lcd.setCursor(15, 2);
    if (seconds < 10)
        lcd.print("0" + countdownStr + "sec");
    else
        lcd.print(countdownStr + "sec");
    lastReadTime = millis();
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
    if (phLevel < PH_TRIGGER_LOW || phLevel > PH_TRIGGER_HIGH)
    {
        if (!pumpIsOn)
        {
            digitalWrite(PUMP_PIN, LOW);
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
