/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/mikesoniat/Documents/Particle/Boron_GPS_Tracker/src/Boron_GPS_Tracker.ino"
/*
 * Project: Boron_GPS_Tracker
 * Description: Boron module with Grove GPS (Air530)
 * Author: Mike Soniat
 * Date: 12/17/2021
 */

#include "TinyGPS++.h"

void setup();
void loop();
#line 10 "/Users/mikesoniat/Documents/Particle/Boron_GPS_Tracker/src/Boron_GPS_Tracker.ino"
SerialLogHandler logHandler;

TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {

    while(Serial1.available())
    {
        if(gps.encode(Serial1.read()))
        {
            String msg = Serial1.readStringUntil('\r');
            Serial.println(msg);

            if (gps.sentencesWithFix() > 0) {
                Serial.print("HAS FIX="); Serial.println(gps.sentencesWithFix());
                Serial.print("LAT="); Serial.println(gps.location.lat());
                Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
                Serial.print("ALT="); Serial.println(gps.altitude.meters(), 6);
                delay(4*1000);

            }

        }
    }  
}

