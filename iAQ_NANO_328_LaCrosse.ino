/*
    I2C BusClock    = 100KHz
    TVOC im Normalbetrieb: 125... 250
    CO2          "  :       400...2500 (?) 

    --- LaCrosse-Protocol for 433 MHz-TX
    --- transfer measured values through LaCrosse-TX2-instance
    --- LaCrosse-object declared as static


*/

#include <Wire.h>
#include <TimerOne\TimerOne.h>
#include "utility/LaCrosse.h"

//--- Primary Sensor ID for FHEM CUL_TX Device receiving
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define LACROSSE_SENSOR_ID_BASIS        92      //--- results in Lacrosse instances with two cannles each for for measument data
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define DELAY_PAUSE_PERIOD              10000  // im ms = 10s, every n seconds a TX-package (incl. 3 Repetitions)   
#define DELAY_PAUSE_PERIOD_SEC          30     // im s = 10s 
#define DELAY_TIMER_CYCLES              10     // im s = 10s 
#define PIN_ENABLE_TIMESTAMP_IN_OUTPUT  10     // jumper nach Masse schaltet Timestamp in der Ausgabe aus.  


#define iaqaddress 0x5A

volatile uint16_t   predict;
volatile uint8_t    statu;
volatile int32_t    resistance;
volatile uint16_t   tvoc;
volatile bool       timer_tick = false; // 

//#define VERBOSE true
//#define USE_STATUS_OUTPUT 
#define USE_PLOT true 

//--- prototypes
void    readAllBytes();
void    checkStatus();
int64_t get_timestamp_us();
void    print_timestamp();

//--------------------------------------------------------------------------------------------
int64_t get_timestamp_us()
{
    //int64_t system_current_time = 0;
    // ...
    // Please insert system specific function to retrieve a timestamp (in microseconds)
    // ...
    return (int64_t)millis() * 1000;
}
//--------------------------------------------------------------------------------------------
void print_timestamp()
{
    //--- a jumper to GND on D10 does the job! 
    if (digitalRead(PIN_ENABLE_TIMESTAMP_IN_OUTPUT) == LOW)
    {
        Serial.print("[");
        Serial.print(get_timestamp_us() / 1e6);
        Serial.print("] ");
    }
}
//----------------------------------------------------------------------------------------------
void timer_callback()
{
    if (timer_tick == true)  return ;  //ignore timer, loop is occupied! 
    timer_tick = true;  //--- do main job in loop()
}
//----------------------------------------------------------------------------------------------
void executeLaCrosseSend()
{
    static uint8_t counter = 0;

    counter++;

    if (digitalRead(PIN_ENABLE_TIMESTAMP_IN_OUTPUT) == LOW && counter > DELAY_TIMER_CYCLES)
    {
        print_timestamp();  Serial.println(F("Timer-Interrupt!"));
    }

    if (counter > DELAY_TIMER_CYCLES)
    {
        counter = 0;

        //*********************************************
        //--- LaCrosse-Protocol for 433 MHz-TX 
        //--- transfer measured values through LaCrosse-TX2-instance
        //--- LaCrosse-object declared as static

        /*Serial.print(predict); Serial.print(",");
        Serial.print(resistance); Serial.print(",");
        Serial.println(tvoc);*/

        LaCrosse.bSensorId = LACROSSE_SENSOR_ID_BASIS;
        
        float co2Reading  = predict / 10.0;     //--- CO2-reading adapts iAQ-CO2-value for protocol needs of LaCrosse-TX3
        float tvocReading = tvoc / 10.0; 

        print_timestamp(); Serial.print("CO2: "); Serial.print(co2Reading); Serial.print("\t TVOC: \t"); Serial.println(tvocReading);

        LaCrosse.t = co2Reading;         //--- alias temperature;  
        LaCrosse.h = tvocReading;    //--- alias humidity;  

        //--- send CO2 reading in PPM as "xxx.xx" value, regarding TX3-protocol restrictions to "temperature"
        LaCrosse.sendTemperature();
        LaCrosse.sleep(1);        /* 1 second, no power-reduction! see impact on powersave */
        
        //--- send TVOV-reading as "xx.xx" value, regarding TX3-protocol restrictions to "humidity".
        LaCrosse.sendHumidity();        
        LaCrosse.sleep(1);        /* 1 second delay, no power-reduction! */
    }
}
//----------------------------------------------------------------------------------------------
void setup()
{
    //  PD1 = SDA = D2
    //  PD0 = SCL = D3

    Serial.begin(115200);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB
        yield(); // reserve for ESP8266
    }

    //Timer1.initialize((unsigned long) DELAY_PAUSE_PERIOD_SEC * 1000 * 1000); // yS in Sekunden
    Timer1.initialize((unsigned long)30000000); // yS in Sekunden
    Timer1.attachInterrupt(timer_callback);

    Serial.println("*** Running.");
    Serial.println("");
    Serial.println("CO2 [ppm] , resistance, TVOC\n");
    
    Wire.begin(iaqaddress);
}
//----------------------------------------------------------------------------------------------
void loop()
{
    readAllBytes();
    #ifdef VERBOSE
        Serial.println("ReadAllBytes done.");
    #endif

    #ifdef USE_STATUS_OUTPUT
        checkStatus();
    #endif 

    #ifdef VERBOSE
        Serial.println("CheckStatus done.");
    #endif


#ifdef USE_PLOT
        Serial.print(predict); Serial.print(",");
        Serial.print(resistance); Serial.print(",");
        Serial.println(tvoc);
#else
    Serial.println();
    Serial.print("CO2:");
    Serial.print(predict);
    Serial.print(", Status:");
    Serial.print(statu, HEX);
    Serial.print(", Resistance:");
    Serial.print(resistance);
    Serial.print(", TVoC:");
    Serial.println(tvoc);
#endif

    if (timer_tick)
    {
        executeLaCrosseSend();
        timer_tick = false;     // reset timer-flag. 
    }
    
    delay(2000);

}
//----------------------------------------------------------------------------------------------
void readAllBytes()
{
    Wire.requestFrom(iaqaddress, 9);

    predict = (Wire.read() << 8 | Wire.read());
    statu = Wire.read();
    resistance = (Wire.read() & 0x00) | (Wire.read() << 16) | (Wire.read() << 8 | Wire.read());
    tvoc = (Wire.read() << 8 | Wire.read());
}
//----------------------------------------------------------------------------------------------
void checkStatus()
{
    if (statu == 0x10)
    {
        Serial.println("Warming up...");
    }
    else if (statu == 0x00)
    {
        Serial.println("Ready");
    }
    else if (statu == 0x01)
    {
        Serial.println("Busy");
    }
    else if (statu == 0x04)
    {
        //--- kommt in der Warmlaufphase ... steht nicht in der Doku!
        Serial.println("BurnIn?");
    }
    else if (statu == 0x80)
    {
        Serial.println("Error");
    }
    else
        Serial.println("No Status, check module");
}
//----------------------------------------------------------------------------------------------
// <eot>
//----------------------------------------------------------------------------------------------