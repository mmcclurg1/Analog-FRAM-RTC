/***********************
M McClurg 5/29/2021
This sketch takes an analog input and a real time clock datapoint and writes them to FRAM memory
builds on the example Adafruit sketch for the 32k FRAM 

at the bottom there are two simple functions that allow numbers 
as high as 65536 to be stored and retrieved by breaking them into 2 bytes.
 

************************/

#include <Wire.h>

// Using an I2C FRAM memory card for non volatile storage of data points andtime stamps
#include "Adafruit_FRAM_I2C.h"

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "Adafruit_RTClib_RK.h"



RTC_DS3231 rtc;

Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;

//We'll use address 0 to hold last FRAM memory location used to store data
unsigned int SaveAddress = 0;

//Variables to hold RTC time stamp information
long Year = 0;
long Date = 0;
long Hour_Min = 0;

unsigned int last_save;
unsigned int writeaddress;

//taking a sample every SAMPLE_DELAY ms and sensing it every SEND_DELAY ms

#define INTERVAL_SAMPLE_DELAY 5000
#define INTERVAL_SEND_DELAY 30000

//Using a 32KB memory card
#define FRAM_SIZE 32767

int loopcounts = 0;

unsigned long sample_time = 0;
unsigned long send_time = 0;
unsigned long voltage1 = 0;
unsigned long total_voltage = 0;
unsigned long avg_voltage =0;

void setup(void) {

  Serial.begin(9600);

  Particle.variable("avg_voltage", avg_voltage);

  // Check to ensure FRAM memory is attached
  if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
      
  } 
  else {
    Serial.println("Long Term Storage not found...contact Load Controls\r\n");
    Serial.println("Will continue in case this processor doesn't support repeated start\r\n");
  }

// Check to ensure RTC is attached
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

   if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  }

    last_save = readMem(0x0);

    // Set last_save to 4 in case something went wrong and memory location 0 is corrupted
    if(last_save > (FRAM_SIZE-8)) last_save = 4;

    writeaddress = last_save;

      delay(10000);   //Slow down so you can see what is written above
  
}

void loop() {
   
       
  while(1) {

        if(millis() >= sample_time + INTERVAL_SAMPLE_DELAY){
        sample_time +=INTERVAL_SAMPLE_DELAY;
 
        // read the input on analog pin 0:
        int sensorValue1 = analogRead(A0);

        // Convert the analog reading (which goes from 409.6 - 4096) to a voltage (.33V to 3.33V), multiply by 1000 to 
        // get a 4 digit interval number between 4000 and 20000
        voltage1 = ((((sensorValue1 - 409.6) / 3686.4 ) * 16.0) + 4.0)* 1000.0;

        total_voltage = total_voltage + voltage1;
        loopcounts++;
        avg_voltage = total_voltage / loopcounts;

        }

        if(millis() >= send_time + INTERVAL_SEND_DELAY){
          send_time += INTERVAL_SEND_DELAY;

        // don't overwrite SaveAddress 0-4 location
        if (writeaddress < 4 || writeaddress > (FRAM_SIZE-8)) writeaddress = 4.0;

        loopcounts = 0;
        total_voltage = 0;

        // read the Real Time Clock for timestamp data
        DateTime now = rtc.now();
        //4 digit year data e.g., 2021
        Year = now.year();
        //4 digit date info e.g., 0529 for May 29
        Date = (now.month()*100) + now.day();
        //4 digit time info e.g., 1228 for 12:28
        Hour_Min = (now.hour()*100) + now.minute();

        // Particle.publish("Year", String(Year), PUBLIC);
        // Particle.publish("Date", String(Date), PUBLIC);
        // Particle.publish("Time", String(Hour_Min), PUBLIC);

        //Serial.print("Year: "); Serial.print(Year);
        //Serial.print(" Date: "); Serial.print(Date);
        //Serial.print(" Time: "); Serial.println(Hour_Min);
        //Serial.println();

        //Write data point and time data to the next FRAM memory location
        // This will be 4 long interval writes, each of 2 bytes
        
        writeMem(writeaddress, avg_voltage);
        writeaddress = writeaddress + 2;
        writeMem(writeaddress, Year);
        writeaddress = writeaddress + 2;
        writeMem(writeaddress, Date);
        writeaddress = writeaddress + 2;
        writeMem(writeaddress, Hour_Min);
  
        // reset writeaddress back to the beginning of this data point and print out stored data to validate
        writeaddress = writeaddress -6;
        unsigned int readValue = readMem(writeaddress);

         Particle.publish("4-20mA", String(avg_voltage), PUBLIC);

         Particle.publish("FRAM read Value", String(readValue), PUBLIC);

         Particle.publish("Writeaddress Location:", String(writeaddress), PUBLIC);

        // Serial.print("writeaddress: "); Serial.print(writeaddress);
        // Serial.print(" 4-20mA: "); Serial.print(voltage1);
        // Serial.print(" FRAM read Value: "); Serial.println(readValue);

        writeaddress = writeaddress + 2;
        readValue = readMem(writeaddress);
        //Particle.publish("Year_mem", String(readValue), PUBLIC);

        // Serial.print(" Year: "); Serial.println(readValue);

        writeaddress = writeaddress + 2;
        readValue = readMem(writeaddress);
        //Particle.publish("Date_mem", String(readValue), PUBLIC);
        // Serial.print(" Date: "); Serial.println(readValue);
                
        writeaddress = writeaddress + 2;
        readValue = readMem(writeaddress);
         Particle.publish("Time_mem", String(readValue), PUBLIC);
        // Serial.print(" Time: "); Serial.println(readValue);

        // move writeaddress ahead 2 to prepare for next data points
        writeaddress = writeaddress + 2;

      // Save the next data address to location 0 in case of power loss  
          writeMem(SaveAddress, writeaddress);  

          last_save = writeaddress;

        }    
  }


}

// Two routines to write and read 2 Byte numbers

  void writeMem(int address, long value) {
    unsigned int MSB = value / 256L;      // Break the value into 2 Byte-parts for storing
    unsigned int LSB = value % 256L;      // Above is MSB, remainder (Modulo) is LSB
    fram.write8(address, MSB);              // Store the value MSB at address add1
    fram.write8(address + 1, LSB);          // Store the value LSB at address add1 + 1
  }

 unsigned int readMem(unsigned int address) {
    unsigned int MSB = fram.read8(address);           //Read the 2 bytes from memory
    unsigned int LSB = fram.read8(address + 1);
    unsigned int value = (256 * MSB) + LSB;
    return value;
  }

void dumpMem(){
      // dump the entire 32K of memory!
      uint8_t value;
      for (uint16_t a = 0; a < 32768; a++) {
       value = fram.read8(a);
       if ((a % 32) == 0) {
         Serial.print("\n 0x"); Serial.print(a, HEX); Serial.print(": ");
       }
       Serial.print("0x");
       if (value < 0x1)
         Serial.print('0');
       Serial.print(value, HEX); Serial.print(" ");
      }

}