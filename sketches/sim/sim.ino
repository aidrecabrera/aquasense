#include <SoftwareSerial.h>
SoftwareSerial sim(10, 11);
int _timeout;
String _buffer;
String number = "+639604377530";
void setup()
{
    delay(7000); // Delay to ensure module gets connection
    Serial.begin(9600);
    _buffer.reserve(50);
    Serial.println("System has started");
    sim.begin(9600);
    delay(1000);
    Serial.println("w - Send SMS, r - Get SMS, c - make a call");
}
void loop()
{
    if (Serial.available() > 0)
        switch (Serial.read())
        {
        case 's':
            SendMessage();
            break;
        case 'r':
            RecieveMessage();
            break;
        case 'c':
            callNumber();
            break;
        }
    if (sim.available() > 0)
        Serial.write(sim.read());
}
void SendMessage()
{
    Serial.println("Sending Message");
    sim.println("AT+CMGF=1"); // Text Mode
    delay(200);
    Serial.println("Set SMS Number");
    sim.println("AT+CMGS=\"" + number + "\"\r");
    delay(200);
    String SMS = "SIM800L IS WORKING!";
    sim.println(SMS);
    delay(100);
    sim.println((char)26); // ASCII code of CTRL+Z
    delay(200);
    _buffer = _readSerial();
}
void RecieveMessage()
{
    Serial.println("SIM800L Read an SMS");
    sim.println("AT+CMGF=1");
    delay(200);
    sim.println("AT+CNMI=1,2,0,0,0"); // AT Command to receive a live SMS
    delay(200);
    Serial.write("Unread Message done");
}
String _readSerial()
{
    _timeout = 0;
    while (!sim.available() && _timeout < 12000)
    {
        delay(13);
        _timeout++;
    }
    if (sim.available())
    {
        return sim.readString();
    }
}
void callNumber()
{
    sim.print(F("ATD"));
    sim.print(number);
    sim.print(F(";\r\n"));
    _buffer = _readSerial();
    Serial.println(_buffer);
}