#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include "Nextion.h"
#include "BH1750.h"
#include "GravityTDS.h"
#include "DHT.h"
#include "ph4502c_sensor.h"

#define soilPin A0
#define pHPin A1
#define tdsPin A2
#define relayPump1Pin 2
#define relayPump2Pin 7
#define relayMistPin 8
#define waterValvePin 3
#define nutritionValvePin 4
#define echoPin 5
#define triggerPin 6
#define dhtPin 9
#define maxDistance 200
#define waterPHPin 10
#define waterTempPin 11
#define pHTriggerPin 12
#define pHCalibration 14.8f
#define pHReadingInterval 100
#define pHReadingCount 10
#define ADCResolution 1024

DHT myDHT(dhtPin, DHT22);
GravityTDS myTDS;
BH1750 luxMeter(0x23);
PH4502C_Sensor PH4502C(waterPHPin, waterTempPin, pHCalibration, pHReadingInterval, pHReadingCount, ADCResolution);

//Monitoring Variables
int soilPercentage;
int waterLevel;
float temperature;
float humidity;
float waterPH;
float waterTemp;
float soilPH;
float tdsValue;
float readLux;

int readSoil;
bool waterState = true;
int readPH;
int Distance = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myDHT.begin();
  Wire.begin();
  luxMeter.begin();
  Serial.println("Machine ON");
  pinMode(relayPump1Pin, OUTPUT);
  pinMode(relayPump2Pin, OUTPUT);
  pinMode(relayMistPin, OUTPUT);
  pinMode(waterValvePin, OUTPUT);
  pinMode(nutritionValvePin, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT); //or use INPUT_PULLUP if trouble
  myTDS.setPin(tdsPin);
  myTDS.setAref(5.0);
  myTDS.setAdcRange(1024);
  myTDS.begin();
  PH4502C.init();
  digitalWrite(triggerPin, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

  //WaterPump for Tank Set-Up use Ultrasonic sensor
  Distance = getDistance();
  waterLevel = map(Distance, 20, 5, 0, 100);
  if (waterLevel < 100 && waterState == true)
    {
      digitalWrite(waterValvePin, HIGH);
      digitalWrite(relayPump2Pin, HIGH);
    }
  else
    {
      digitalWrite(relayPump2Pin, LOW);
      digitalWrite(waterValvePin, LOW);
      waterState = false;

      if (waterLevel <= 10)
        {
          waterState = true;
        }
    }

  //Monitoring myTDS and Setting up the Nutrition use TDS sensor
  myTDS.setTemperature(temperature);
  myTDS.update();
  tdsValue = myTDS.getTdsValue();
  if (waterLevel == 100 && tdsValue < 1000 && waterState == false)
    {
      digitalWrite(nutritionValvePin, HIGH);
      digitalWrite(relayPump2Pin, HIGH);
    }
  else
    {
      digitalWrite(relayPump2Pin, LOW);
      digitalWrite(nutritionValvePin, LOW);
    }
  
  //MistMaker and DHT Set-Up use DHT22 sensor
  temperature = myDHT.readTemperature();
  humidity = myDHT.readHumidity();
  if (humidity <= 70)
    {
      digitalWrite(relayMistPin, HIGH);
    }
  else
    {
      digitalWrite(relayMistPin, LOW);
    }

  //Monitoring Soil pH use Soil pH sensor
  readPH = analogRead(pHPin);
  soilPH = (-0.0693 * readPH) + 7.3855;

  //Monitoring Light Intensity use BH1750
  readLux = luxMeter.readLightLevel();

  //Monitoring pH and Temp of Water and Nutrition use PH4502C
  waterPH = PH4502C.read_ph_level();
  waterTemp = PH4502C.read_temp();
  
  //WaterPump for Hydroponics Set-Up use Soil Moist sensor
  readSoil = analogRead(soilPin);
  soilPercentage = map(readSoil, 0, 1023, 0, 100);
  if (soilPercentage <= 70)
    {
      digitalWrite(relayPump1Pin, HIGH);
    }
  else
    {
      digitalWrite(relayPump1Pin, LOW);
    }
}

//Ultrasonic Function
int getDistance() {

  long duration = 0;
  int distance = 0;
  int watchloop = 0;

  // Clear the triggerPin by setting it LOW and wait for any pulses to expire:
  digitalWrite(triggerPin, LOW);      // setting state to low and
  delay(2);                           // waiting 2,000uS (or 686cm (>22ft) to eliminate "echo noise")

  // only grab values under 20ft/610cm (for some reason, 676 is a common return error for ∞ distance)
  while ( (distance == 0) || (distance > 610) ) {
    // Trigger the sensor by setting the triggerPin high for 10 microseconds:
    digitalWrite(triggerPin, HIGH);         // start sending the 40kHz wave...
    delayMicroseconds(20);                  // sending for 20uS
    digitalWrite(triggerPin, LOW);          // stop sending 40kHz wave

    // Read the echoPin. pulseIn() duration of when the wave-echo stops (in microseconds):
    duration = pulseIn(echoPin, HIGH);

    // Calculate the distance:
    distance = duration * 0.034 / 2;
    //Serial.print("distance=");Serial.println(distance);

    // Catch funky ∞ distance readings
    watchloop++;        
    if (watchloop > 20){      // If errant "676" readings 20 times
      distance = 610;         // set distance to 610cm (20ft) 
      break;                  // and break out of loop (not really needed if forced to 610)
    }
    
  }

  return (distance);
}
