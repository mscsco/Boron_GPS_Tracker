/*
 * Project: Boron_GPS_Tracker
 * Description: Boron module with Grove GPS (Air530)
 * Author: Mike Soniat
 * Date: 12/17/2021
 */

SerialLogHandler logHandler;

unsigned char buffer[64];  // buffer array for data receive over serial port
int count=0;    
const pin_t MY_LED = D7;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(MY_LED, OUTPUT);
}

void loop() {


    if (Serial1.available())                     // if data is coming from serial1 port ==> data is coming from GPS module
    {
        while(Serial1.available())               // reading data into char array
        {
            digitalWrite(MY_LED, HIGH);
            buffer[count++]=Serial1.read();      // writing data into array
            if(count == 64)break;
        }
        digitalWrite(MY_LED, LOW);
        
        //convert buffer to string 
        String myString = (char*)buffer;
        Serial.print(myString);
        
        //Serial.write(buffer,count);                 // if no data transmission ends, write buffer to hardware serial port
        clearBufferArray();                         // call clearBufferArray function to clear the stored data from the array
        count = 0;                                  // set counter of while loop to zero 
    }
    if (Serial.available())                 // if data is available on hardware serial port ==> data is coming from PC or notebook
    Serial1.write(Serial.read());        // write it to the GPS module

}

void clearBufferArray()                     // function to clear buffer array
{
    for (int i=0; i<count;i++)
    {
        buffer[i]=NULL;
    }                      // clear all index of array with command NULL
}
