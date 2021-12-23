/*
 * Project: Boron_GPS_Tracker
 * Description: A prototype for tracking the location and operation of heavy equipment.
 * Author: Mike Soniat
 * Company: MSCS Technology, LLC
 * Date: 12/19/2021
 * Components:  Particle Boron LTE CAT-M1 Cellular w/ EtherSIM
 *              Grove GPS (Air530) module for location tracking (UART)
 *              Grove BME280 Temperature/humidity sensors for monitoring environmental conditions (I2C_2)
 *              Grove SW-420 Vibration sensor to determine when the equipment is running vs idle
 * 
 * Libraries:   AdafruitBME280; for Grove temp/humidity sensor
 *              TinyGPS++; Port of TinyGPS for the Particle AssetTracker; https://github.com/mikalhart/TinyGPSPlus
 *              JsonParserGeneratorRK; parses JSON for events
 *              Ubidots; sends data to Ubidots.com
 */

#include "TinyGPS++.h"
#include "Adafruit_BME280.h"
#include "JsonParserGeneratorRK.h"
#include "Ubidots.h"

SerialLogHandler logHandler;

TinyGPSPlus gps;
Adafruit_BME280 bme;
FuelGauge fuel;

//environment vars
int temp_c = 0;
int temp_f = 0;
int humidity = 0;

//device vars
int voltage = 0;
int signal_strength  = 0;
int signal_quality = 0;
int percent_charge = 0;

//location vars
double longitude = 0.0;
double latitude = 0.0;
double altitude = 0.0;

//last readings for comparison
int last_temp_c = 1;
int last_humidity = 1;
int last_voltage = 1;
int last_longitude = 1;
int last_latitude = 1;

//loop vars
bool first_loop = true;
unsigned long delay_millis = 300000;
unsigned long lastCheck = 0;

//Ubidots vars
const char *WEBHOOK_NAME = "Ubidots";
char webhook[] = "webhook";
Ubidots ubidots(webhook, UBI_PARTICLE);

// product/version
PRODUCT_ID(16112)
PRODUCT_VERSION(4)

void setup() {
    //setup serial port
    Serial.begin(9600);
  
    //setup GPS port
    Serial1.begin(9600);

    //setup BME sensor
    bme.begin();

    //setup Particle Variables
    Particle.variable("temp_f", temp_f);
    Particle.variable("humidity", humidity);
    Particle.variable("signal_strength", signal_strength);
    Particle.variable("signal_quality", signal_quality);
    Particle.variable("percent_charge", percent_charge);    
    Particle.variable("longitude", &longitude, DOUBLE);
    Particle.variable("latitude", &latitude, DOUBLE);
    Particle.variable("altitude", &altitude, DOUBLE);

}

void loop() {
    unsigned long currentMillis = millis();

    if((currentMillis - lastCheck > delay_millis) | first_loop) //after first loop, wait delay_millis to check again
    {
        lastCheck = currentMillis;
        
        temp_c = (int8_t)bme.readTemperature();
        temp_f = (temp_c * 1.8) + 32;
        humidity = (uint8_t)bme.readHumidity();
        voltage = (uint8_t)fuel.getVCell();
        percent_charge = (uint8_t)System.batteryCharge();

        CellularSignal sig = Cellular.RSSI();
        signal_strength  = (uint8_t)sig.getStrength();
        signal_quality = (uint8_t)sig.getQuality();

        // get GPS coordinates
        getGPS();

        first_loop = false;
        delay(5000);

        if((last_temp_c != temp_c) | (last_humidity != humidity) | (last_latitude != latitude) | (last_longitude != longitude))
        {
            createEventPayload(temp_c, temp_f, humidity, voltage, percent_charge, signal_strength , signal_quality, longitude, latitude, altitude);
            last_temp_c = temp_c;
            last_humidity = humidity;
            last_latitude = latitude;
            last_longitude = longitude;
        }
    }
    
}

void getGPS() {
    while(Serial1.available())
    {
        if(gps.encode(Serial1.read()))
        {
            String msg = Serial1.readStringUntil('\r');
            Serial.println(msg);

            if (gps.sentencesWithFix() > 0) {
                latitude = gps.location.lat();
                longitude = gps.location.lng();
                altitude = gps.altitude.feet();
            }

        }
    }  

}

void createEventPayload(int temp_c, int temp_f, int humidity, int voltage, int percent_charge, int signal_strength , int signal_quality, double longitude, double latitude, double altitude)
{
  JsonWriterStatic<256> jw;

  {
    JsonWriterAutoObject obj(&jw);

    jw.insertKeyValue("temp_c", temp_c);
    jw.insertKeyValue("temp_f", temp_f);
    jw.insertKeyValue("humidity", humidity);
    jw.insertKeyValue("voltage", voltage);
    jw.insertKeyValue("percent_charge", percent_charge);
    jw.insertKeyValue("signal_strength", signal_strength );
    jw.insertKeyValue("signal_quality", signal_quality);
    jw.insertKeyValue("longitude", longitude);
    jw.insertKeyValue("latitude", latitude);
    jw.insertKeyValue("altitude", altitude);

  }

  Particle.publish("equipment_readings", jw.getBuffer(), PRIVATE);

  //add Ubidots variables
  getLocation();

}

void getLocation() {

    /* Reserves 10 bytes of memory to store context keys values, add as much as needed */
    char *str_lat = (char *)malloc(sizeof(char) * 10);
    char *str_lng = (char *)malloc(sizeof(char) * 10);

    /* Saves the coordinates as char*/
    sprintf(str_lat, "%f", latitude);
    sprintf(str_lng, "%f", longitude);

    /* Reserves memory to store context array */
    char *context = (char *)malloc(sizeof(char) * 50);

    /* Adds context key-value pairs */
    char latLabel[] = "lat";
    char lngLabel[] = "lng";
    ubidots.addContext(latLabel, str_lat);
    ubidots.addContext(lngLabel, str_lng);

    /* Builds the context with the coordinates to send to Ubidots */
    ubidots.getContext(context);

    /* Sends the position */
    char positionLabel[] = "position";
    ubidots.add(positionLabel, 0, context); // Change for your variable name

    //other vars
    char tempLabel[] = "Temperature in Fahrenheit";
    char humidityLabel[] = "Humidity";
    char batteryLabel[] = "Battery";

    ubidots.add(tempLabel, temp_f);
    ubidots.add(humidityLabel, humidity);
    ubidots.add(batteryLabel, percent_charge);

    bool bufferSent = false;
    bufferSent = ubidots.send(WEBHOOK_NAME, PUBLIC); // Will use particle webhooks to send data

    if (bufferSent)
    {
        // Do something if values were sent properly
        Serial.println("Values sent by the device");
    }

    /* frees memory */
    free(str_lat);
    free(str_lng);
    free(context);
}

