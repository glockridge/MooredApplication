
#include <JeeLib.h> // Low power functions library 
#include<LowPower.h>
#include <SD.h>                  // Load the SD card library
#include <SPI.h>                 // Load the serial library
#include <Wire.h>                //Load the I2C library
#include <SoftwareSerial.h>      //Load the Software Serial Library. This library in effect gives the arduino additional serial ports
#include <elapsedMillis.h>       //Load the timer library
#include <RTClib.h>
RTC_DS1307 RTC;                  //the RTC on the adafruit logging shield


SoftwareSerial myserial(8, 7);   //THIS IS FOR UNO//define how the soft serial port is going to work.
//SoftwareSerial mySerial(52, 7); //Initialize SoftwareSerial, and tell it you will be connecting through pins (x,y)
//Apparently, the MEGA does not support softwareSerail on pins 8,7 like the UNO, RX on the MEGA is supported on different pins
// I chose 52, because I'm badass like that!
//#define mySerial Serial1         // Use this line if you are using an Arduino Mega



unsigned var;                    //variable used in powerdown loop, needs to be defined somewhere
int chipSelect = 10;             //chipSelect pin is set to 10 to keep the SD card reader happy... 

File logfile;


int cond_address = 100;          //address of the atlas EC microchip on the I2C bus... On the MEGA SCL is pin 21 connects to RX on the chip, SDA is pin 20 and connects to TX on the chip...Make sure to connect 10k resistors from TX and RX on the chip to +5v in addition to pins 20 and 21  
byte code=0;                     //used to hold the I2C response code. 
char ec_data[48];                //we make a 48 byte character array to hold incoming data from the EC circuit. 
byte in_char=0;                  //used as a 1 byte buffer to store in bound bytes from the EC Circuit.   
byte i=0;                        //counter used for ec_data array. 
int time=1400;                   //used to change the delay needed depending on the command sent to the EZO Class EC Circuit.  

char *ec;                        //char pointer used in string parsing. 
char *tds;                       //char pointer used in string parsing.
char *sal;                       //char pointer used in string parsing.
char *sg;                        //char pointer used in string parsing.

float ec_float;                  //float var used to hold the float value of the conductivity. 
float tds_float;                 //float var used to hold the float value of the TDS.
float sal_float;                 //float var used to hold the float value of the salinity.
float sg_float;                  //float var used to hold the float value of the specific gravity.

float tempC;                     //float var used to hold the temperatre reading.
float temp_data;


void setup()  
{
  Serial.begin(9600);
  Wire.begin();

  
  pinMode(10, OUTPUT);  //setting the digital pin 10 as an output to keep the SD card reader happy, need to check for mega and my sd logger...may be 53 for mega
  pinMode(4, OUTPUT);  //setting the temperatre probe voltage supply as an output.
  

  
  if (!SD.begin(chipSelect,11,12,13)) {                   //This is helpful for debuging, I recommend leaving this in the code...not sure why this line will not work, but i would also expect it to be 50,51,52 instead of 10,11,12
    Serial.println("Card init. failed!");
    }
  //if                                                     //Uncomment these lines to create a new file each time the device cycles power.
 // char filename[15];                             
//strcpy(filename, "1Drift.csv");  
//  for (uint8_t i = 0; i < 100; i++) {
//    filename[6] = '0' + i/10;
//    filename[7] = '0' + i%10;
//    if (! SD.exists(filename)) {                // create if does not exist, do not open existing, write, sync after write
//    break;
//  }}

 // logfile = SD.open(filename, FILE_WRITE);     //open a file and label the columns
 
 char filename[15];                             
strcpy(filename, "FOWLWT.csv");  //CHANGE THE DRIFTER NUMBER HERE, DO NOT CHANGE THE FORMAT, THE PROGRAM GETS PISSY IF  YOU START ALTERING FILNAMES!!


// if (! SD.exists(filename)){  //if this filename do not already exist, then add headers, else, don't add headers, open the file and append data. removing this line will add headers everytime a sample is logged after power cycling...may end up with multiple headers but better then not having them
      //Serial.println ("adding headers"); //this progrma only adds headers once the first sample is taken. Not a problem when sampling every 10 seconds, but with long intervals like I have headers are not added a lot. 
      
     logfile= SD.open(filename, FILE_WRITE);  //This prevents multiple files from being written, instead, it creates a single file and writes to it over and over.
          logfile.print(F("Date/Time"));      //F stores values in flash needed for UNO 
          logfile.print(F(","));
          logfile.print(F("T(C)"));
          logfile.print(F(","));
          logfile.print(F("Cond"));
          logfile.print(F(","));
          logfile.print(F("TDS"));
          logfile.print(F(","));
          logfile.print(F("Salinity"));
          logfile.print(F(","));
          logfile.println(F("Sp_G"));
         
//        }
//else{
//logfile= SD.open(filename,FILE_WRITE);
    //}  

 
 
}//End of the void setup function!




void loop () //heres where im fucking everything up in my deletions
 
          {                 
              DateTime now;  //tell chip what time it is
            now = RTC.now();


           if ((now.minute()==0) | (now.minute()==30)) {   // choose any times you want, I like 0 and 30 to keep in line with mobile Bay
            
            
            readEC();
                 
                  logfile.println(ec_data);            //printing data from the atlas EC microchip!!
                  Serial.println(ec_data);
                  logfile.flush();   
     
                var = 0;
                while(var < 210){
                   // do something 210 times
               var++;
                  //put the processor to sleep for 8 seconds
                   LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
                  //LowPower.idle(SLEEP_4S, ADC_OFF, TIMER5_OFF, TIMER4_OFF, TIMER3_OFF,  
                      //TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART3_OFF,  
                      //USART2_OFF, USART1_OFF, USART0_OFF, TWI_OFF); 

                }                   

        
}  
            

            else {  Wire.beginTransmission(cond_address);  //this line turns the EC chip off if power cycles then it will stay on without this line unitl if statement is met
                    Wire.write("Sleep");
                    Wire.endTransmission();
        
              
              LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);} //sleeps for 8 seconds then wakes up to see if condition is met

          }




void readEC() {
float v_out;
float tempC;
char tempData[20];  //holding the value of tempC as a character.
digitalWrite(A0, LOW);  //White wire from the temp sensor

digitalWrite(4, HIGH);  //Red wire from the temp sensor
delay(2);
v_out=analogRead(A0);
digitalWrite(4, LOW);

v_out*=0.0048;

v_out*=1000;
tempC=0.0512*v_out-21.2000;  // CALIBRATE TEMP BY CHANGING END INTERGER HERE...  factory reset calibration 20.5128

  DateTime now;
  now = RTC.now();
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print(" ");
    logfile.print(now.hour(), DEC);
    logfile.print(":");
    logfile.print(now.minute(), DEC);
    logfile.print(",");  
      
      Serial.println(tempC);
      logfile.print(tempC);
      logfile.print(",");
      dtostrf(tempC,4,3,tempData);  //This is the magic line of code that converts the float TempC into a char, that can then be sent to the EC chip via I2C.  4 is the number of width, and three is the precision or number of decimal places.  This was really fucking hard to figure out... but it's my bitch now.
  
  
           Wire.beginTransmission(cond_address);  // Begin communication via I2C with the atlas EC microchip
           delay(15);            // This delay is necessary after initiating I2C communication  
      Wire.write("T,");
     Wire.write(tempData);  //SENDING TEMP C IS NOT A VALUE THAT WORKS>>> YOU NEED TO SEND ACTUAL READING
      delay(300);
      // Wire.requestFrom(cond_address,6,1);
      // while(Wire.available()){
      //   char c=Wire.read();
         //Serial.print(c);
        // }
         
      Wire.endTransmission(); 
 
//delay(200);


           Wire.beginTransmission(cond_address);  // Begin communication via I2C with the atlas EC microchip
           delay(15); 
           Wire.write('r');                       //Sending the atlas EC microchip a command requesting a single reading
           Wire.endTransmission();                //end I2C transmission and wait for response from atlas EC microchip
           delay(1400);                           //This delay is the amount of time required by the atlas EC microchip to take a single EC reading  
           Wire.requestFrom(cond_address,48,1);   //cond_address is definded at the begining of the program as 100.  48 is the number of bytes that are requested, actual byte count will be smaller

       while(Wire.available()) {
           in_char=Wire.read();                 //code speak for breaking the four readings that are recieved from the atlas EC microchip into a form that can be understood via serial/SD 
          
           ec_data[i]=in_char;
          
           i+=1;
           if (in_char==0) {
           
           i=0;
           Wire.endTransmission();
           
           break; 
           
           }
            
           Wire.beginTransmission(cond_address);
           delay(15);
           Wire.write("Sleep");
           delay(300);
           Wire.endTransmission();
          delay(20); 
           }
 }


