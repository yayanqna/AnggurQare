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
BH1750 luxMeter(0x23); //or change to 0x5C if trouble
PH4502C_Sensor PH4502C(waterPHPin, waterTempPin, pHCalibration, pHReadingInterval, pHReadingCount, ADCResolution);

//Nextion HMI component for Waveform
NexWaveform soilPHWave = NexWaveform (1,1,"s0"); 
NexWaveform soilMoistureWave = NexWaveform (2,1,"s0"); 
NexWaveform nutritionWave = NexWaveform (3,1,"s0"); 
NexWaveform waterPHWave = NexWaveform (4,1,"s0"); 
NexWaveform waterTempWave = NexWaveform (5,1,"s0"); 
NexWaveform waterLevelWave = NexWaveform (6,1,"s0"); 
NexWaveform temperatureWave = NexWaveform (7,1,"s0"); 
NexWaveform lightnessWave = NexWaveform (8,1,"s0"); 
NexWaveform humidityWave = NexWaveform (9,1,"s0"); 

//Monitoring Variables
float soilPercentage;
float waterLevel;
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
String sendToESP = "";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(115200);
  nexInit();
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
  //Monitoring Soil pH use Soil pH sensor
  readPH = analogRead(pHPin);
  soilPH = (-0.0693 * readPH) + 7.3855;
  Serial.print("mainPage.x0.val=");
  Serial.print(soilPH);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  soilPHWave.addValue(0, readPH);

  //Monitoring Light Intensity use BH1750
  readLux = luxMeter.readLightLevel();
  Serial.print("mainPage.x7.val=");
  Serial.print(readLux);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  lightnessWave.addValue(0, readLux);

  //Monitoring pH and Temp of Water and Nutrition use PH4502C
  waterPH = PH4502C.read_ph_level();
  Serial.print("mainPage.x3.val=");
  Serial.print(waterPH);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  waterPHWave.addValue(0, waterPH);

  waterTemp = PH4502C.read_temp();
  Serial.print("mainPage.x4.val=");
  Serial.print(waterTemp);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  waterTempWave.addValue(0, waterTemp);

  //WaterPump for Tank Set-Up use Ultrasonic sensor
  Distance = getDistance();
  waterLevel = map(Distance, 20, 5, 0, 100);
  Serial.print("mainPage.x5.val=");
  Serial.print(waterLevel);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  waterLevelWave.addValue(0, waterLevel);

  //Monitoring myTDS and Setting up the Nutrition use TDS sensor
  myTDS.setTemperature(waterTemp); //or use "temperature" from DHT22
  myTDS.update();
  tdsValue = myTDS.getTdsValue();
  Serial.print("mainPage.x2.val=");
  Serial.print(tdsValue);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  nutritionWave.addValue(0, tdsValue);

  //MistMaker and DHT Set-Up use DHT22 sensor
  temperature = myDHT.readTemperature();
  Serial.print("mainPage.x6.val=");
  Serial.print(temperature);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  temperatureWave.addValue(0, temperature);

  humidity = myDHT.readHumidity();
  Serial.print("mainPage.x8.val=");
  Serial.print(humidity);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  humidityWave.addValue(0, humidity);

  //WaterPump for Hydroponics Set-Up use Soil Moist sensor
  readSoil = analogRead(soilPin);
  soilPercentage = map(readSoil, 0, 1023, 100, 0);
  Serial.print("mainPage.x1.val=");
  Serial.print(soilPercentage);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  soilMoistureWave.addValue(0, soilPercentage);

  //send data to ESP
  sendToESP = "";
  sendToESP += soilPH;
  sendToESP += ";";
  sendToESP += readLux;
  sendToESP += ";";
  sendToESP += waterPH;
  sendToESP += ";";
  sendToESP += waterTemp;
  sendToESP += ";";
  sendToESP += waterLevel;
  sendToESP += ";";
  sendToESP += tdsValue;
  sendToESP + ";";
  sendToESP += temperature;
  sendToESP += ";";
  sendToESP += humidity;
  sendToESP += ";";
  sendToESP += soilPercentage;
  Serial1.println(sendToESP);

  //Logic for fill Water Tank
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

  //Logic for Refill Nutrition 
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
  
  //Logic for Mist Maker to humid the environment
  if (humidity < 70 && tdsValue > 900 && waterState == false)
    {
      digitalWrite(relayMistPin, HIGH);
    }
  else
    {
      digitalWrite(relayMistPin, LOW);
    }
  
  //Logic for Watering the Hydroponics
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
