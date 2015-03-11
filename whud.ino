#include <SoftwareSerial.h>
#include <LedControl.h>

#define RXPIN 9
#define TXPIN 8
#define BAUDRATE 19200  
#define DEBUG
#define WORD_LENGTH 4

#include <ELM327.h>

LedControl mydisplay = LedControl(11, 10, 12, 1);
Elm327 Elm(RXPIN, TXPIN, BAUDRATE);
byte status = 0;
bool doClean = false;

static void clean(int pos)
{
    mydisplay.setChar(0, 7 - pos, ' ', false);
}

static void update(int startPos, int number)
{
    int numLength = 0;
    int _number = number;

    while (_number != 0)
    {
        _number /= 10;
        numLength ++;
    }

    char num[] = {0, 0, 0, 0};
    for (int i = 0; i < WORD_LENGTH; ++i)
    {
        int bit = WORD_LENGTH - 1 - i; 
        int a = pow(10, i);
        int b = number / a;
        int c = b % 10;
        num[bit] = ((number / (int)pow(10, i)) % 10);
        if (i != 0 && num[bit] == 0 && i >= (numLength - 1))
            mydisplay.setChar(0, 7 - (i + startPos), ' ', false);
        else
            mydisplay.setDigit(0, i + startPos, num[bit], false);
    }
}

static void ELM327()
{
    int rpm = 0;
    status = Elm.engineRPM(rpm);

    if (status != ELM_SUCCESS){
        Serial.print("rpm Error: ");
        Serial.println(status);
    }

    Serial.print("rpm = ");
    Serial.println(rpm);

    if (rpm == 0)
    {
        if (doClean)
        {
            for (int i = 0; i < 8; ++i)
            {
                clean(i);
            }
            doClean = false;
        }
        return;
    }
    doClean = true;

    byte speed = 0;
    status = Elm.vehicleSpeed(speed);

    if (status != ELM_SUCCESS){
        Serial.print("speed Error: ");
        Serial.println(status);
    }

    Serial.print("speed = ");
    Serial.println(speed);

    update(0, speed);
    update(4, rpm);
}

void setup() {
    mydisplay.shutdown(0, false);  // turns on display
    mydisplay.setIntensity(0, 15); // 15 = brightest
#if defined(DEBUG)
    Serial.begin(115200);
#endif
    Serial.println("ready");
    while (ELM_SUCCESS != (status = Elm.begin()))
    {
        if (status != ELM_SUCCESS){
            Serial.print("Begin Error: ");
            Serial.println(status);
        }
        else
            Serial.println("begin ok");
    }

}

void loop() {
    ELM327();

    delay(10);
}

