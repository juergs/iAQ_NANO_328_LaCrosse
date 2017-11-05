//#include <IAQ2000\I2Cdev.h>
//#include <IAQ2000\IAQ2000.h>
#include <Wire.h>

#define iaqaddress 0x5A

uint16_t  predict;
uint8_t   statu;
int32_t   resistance;
uint16_t  tvoc;


// sensor readings structure after conversion and derivation
struct sensorsT 
{
    // iAQ Core-C
    byte AQ_status;           // 0-ok,1-busy,16-warming up,128-error
    unsigned int CO2eqP;      // predicted CO2eq ppm
    unsigned long Resistance; // sensor resistance ohms
    unsigned int Tvoc;        // predicted TVOCeq ppb
                              // HIH6131
    byte RhT_status;          // 0-normal,1-stale data (already fetched), 2-in CMD mode, 3-n/u
    float Rh;                 // relative humidity
    float Temp;               // temperature
};
typedef struct sensorsT sensors;

sensors sensor;

//--- prototypes
void readAllBytes();
void checkStatus();
int64_t get_timestamp_us();

//--------------------------------------------------

int64_t get_timestamp_us()
{
    return (int64_t)millis() * 1000;
}
//--------------------------------------------------
void checkStatus()
{
    if (sensor.AQ_status == 0x10)
    {
        Serial.println("Warming up...");
    }
    else if (sensor.AQ_status == 0x00)
    {
        Serial.println("Ready");
    }
    else if (sensor.AQ_status == 0x01)
    {
        Serial.println("Busy");
    }
    else if (sensor.AQ_status == 0x80)
    {
        Serial.println("Error");
    }
    else
        Serial.println("No Status, check module");
}
//--------------------------------------------------
void setup()
{
    //  PD1 = SDA = D2
    //  PD0 = SCL = D3

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); 

    Wire.begin();

    Serial.begin(115200);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB
        yield();
    }

    delay(500);

    Serial.println("*** Running now.");
    Serial.println("Resistance, Status, TVOC, CO2eqPpm");
}
//--------------------------------------------------
void loop()
{
    delay(200);

    digitalWrite(LED_BUILTIN, HIGH);

    
    
   /* readAllBytes();
    Serial.println("ReadAllBytes done.");

    checkStatus();
    Serial.println("CheckStatus done.");

    Serial.println();
    Serial.print("CO2:");
    Serial.print(predict);
    Serial.print(", Status:");
    Serial.print(statu, HEX);
    Serial.print(", Resistance:");
    Serial.print(resistance);
    Serial.print(", TVoC:");
    Serial.println(tvoc);*/
    

    getAQ();

    checkStatus();

    int64_t timestamp = get_timestamp_us(); 

    Serial.print("[");
    Serial.print(timestamp / 1e6);
    Serial.print("] \t");
   
   // Serial.println("Resistance, Status, TVOC, CO2eqPpm"); 
    Serial.print(sensor.Resistance);
    Serial.print(" , ");

    Serial.print(sensor.AQ_status);
    Serial.print(" , ");

    Serial.print (sensor.Tvoc);
    Serial.print(" , ");

    Serial.print(sensor.CO2eqP);
    Serial.println("");
        
    digitalWrite(LED_BUILTIN, HIGH);

   /* for (uint8_t i=0;i<4; i++)
    {
        _delay_ms (500);
        yield();
    }*/

    delay(200);

    digitalWrite(LED_BUILTIN, LOW);
}
//--------------------------------------------------
void readAllBytes()
{
    Wire.requestFrom(iaqaddress, 9);

    predict = (Wire.read() << 8 | Wire.read());
    statu = Wire.read();
    resistance = (Wire.read() & 0x00) | (Wire.read() << 16) | (Wire.read() << 8 | Wire.read());
    tvoc = (Wire.read() << 8 | Wire.read());
}
//--------------------------------------------------
//void checkStatus()
//{
//    if (statu == 0x10)
//    {
//        Serial.println("Warming up...");
//    }
//    else if (statu == 0x00)
//    {
//        Serial.println("Ready");
//    }
//    else if (statu == 0x01)
//    {
//        Serial.println("Busy");
//    }
//    else if (statu == 0x80)
//    {
//        Serial.println("Error");
//    }
//    else
//        Serial.println("No Status, check module");
//}

//------------------------------------------------------

void boardTest()
{
    //  PD1 = SDA = D2
    //  PD0 = SCL = D3

    digitalWrite(3, HIGH);
    digitalWrite(2, HIGH);
    delay(1);
    digitalWrite(3, LOW);
    digitalWrite(2, LOW);
    delay(1);

}

//--------------------------------------
// read iAQ Core-C 
void getAQ() 
{
    byte CO2predL, CO2predH, iAQstatus, reszero, res16, res8, res0, tvocH, tvocL;
    // ask the iAQ for data
    // NOTE: Do not communicate with the write bit set just read the bytes
    Wire.requestFrom(iaqaddress, 9);
    CO2predH = Wire.read();
    CO2predL = Wire.read();
    iAQstatus = Wire.read();
    reszero = Wire.read();  // always zero
    res16 = Wire.read();
    res8 = Wire.read();
    res0 = Wire.read();
    tvocH = Wire.read();
    tvocL = Wire.read();
    // assemble the values for the structure (could be done with the reads)
    sensor.AQ_status = iAQstatus;
    sensor.CO2eqP = (CO2predH << 8) | CO2predL;
    sensor.Resistance = (res16 << 16) | (res8 << 8) | res0;
    sensor.Tvoc = (tvocH << 8) | tvocL;
}