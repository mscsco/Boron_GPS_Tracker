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
 *              Ubidots; sends data to Ubidots.com
 */

#include "TinyGPS++.h"
#include "Adafruit_BME280.h"
#include "Ubidots.h"

SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;

//forward declarations
void getInfo();
void pushUbidots();

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

//Ubidots vars
const char *WEBHOOK_NAME = "Ubidots";
char webhook[] = "webhook";
Ubidots ubidots(webhook, UBI_PARTICLE);

//forward declarations
void displayInfo(); 
void pushUbidots();

//gps loop vars
const unsigned long PUBLISH_PERIOD = 30000;
const unsigned long SERIAL_PERIOD = 10000;
const unsigned long MAX_GPS_AGE_MS = 10000;
unsigned long lastSerial = 0;
unsigned long lastPublish = 0;
unsigned long startFix = 0;
bool gettingFix = false;

// product/version
PRODUCT_ID(16112)
PRODUCT_VERSION(5)

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
    Particle.variable("latitude", latitude);

    //from test project
    startFix = millis();
    gettingFix = true;

}

void loop()
{

    while (Serial1.available() > 0) {
        if (gps.encode(Serial1.read())) {
            displayInfo();
        }
    }

}

void displayInfo()
{
    if (millis() - lastSerial >= SERIAL_PERIOD) {
        lastSerial = millis();

        //get other vars
        temp_c = (int8_t)bme.readTemperature();
        temp_f = (temp_c * 1.8) + 32;
        humidity = (uint8_t)bme.readHumidity();
        voltage = (uint8_t)fuel.getVCell();
        percent_charge = (uint8_t)System.batteryCharge();

        CellularSignal sig = Cellular.RSSI();
        signal_strength  = (uint8_t)sig.getStrength();
        signal_quality = (uint8_t)sig.getQuality();        

        char buf[128];
        char pubbuf[240];
        if (gps.location.isValid() && gps.location.age() < MAX_GPS_AGE_MS) {

            snprintf(buf, sizeof(buf), "%f", gps.location.lat());
            snprintf(buf, sizeof(buf), "%f", gps.location.lng());

            latitude = gps.location.lat();
            longitude = gps.location.lng();

            snprintf(buf, sizeof(buf), "%f,%f,%f", gps.location.lat(), gps.location.lng(), gps.altitude.meters());
            snprintf(pubbuf, sizeof(pubbuf), "{\"temp_f\":\"%d\",\"humidity\":\"%d\",\"percent_charge\":\"%d\",\"signal_strength\":\"%d\",\"signal_quality\":\"%d\",\"position\": {\"value\":1, \"context\":{\"lat\": \"%f\", \"lng\": \"%f\"}}}", temp_f, humidity, percent_charge, signal_strength, signal_quality, gps.location.lat(), gps.location.lng());
            
            Serial.println(pubbuf);
            
            if (gettingFix) {
                gettingFix = false;
                unsigned long elapsed = millis() - startFix;
                Serial.printlnf("%lu milliseconds to get GPS fix", elapsed);
            }
        }
        else {
            strcpy(buf, "no location");
            snprintf(pubbuf, sizeof(pubbuf), "{\"temp_f\":\"%d\",\"humidity\":\"%d\",\"percent_charge\":\"%d\",\"signal_strength\":\"%d\",\"signal_quality\":\"%d\",\"position\": {\"value\":1, \"context\":{\"lat\": \"%f\", \"lng\": \"%f\"}}}", temp_f, humidity, percent_charge, signal_strength, signal_quality, gps.location.lat(), gps.location.lng());
            if (!gettingFix) {
                gettingFix = true;
                startFix = millis();
            }
        }
        Serial.println(buf);

        if (Particle.connected()) {
            if (millis() - lastPublish >= PUBLISH_PERIOD) {
                lastPublish = millis();
                Particle.publish("gps", pubbuf, PRIVATE);
                void pushUbidots();
            }
        }
    }

}

void pushUbidots() {

    /* Reserves 10 bytes of memory to store context keys values, add as much as needed */
    char *str_lat = (char *)malloc(sizeof(char) * 10);
    char *str_lng = (char *)malloc(sizeof(char) * 10);

    /* Saves the coordinates as char*/
    sprintf(str_lat, "%f", gps.location.lat());
    sprintf(str_lng, "%f", gps.location.lng());

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
    char positionLabel[] = "location";
    ubidots.add(positionLabel, 0, context); // Change for your variable name

    //other vars
    char tempLabel[] = "Temperature";
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