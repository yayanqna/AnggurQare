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
PH4502C_Sensor PH4502C(
  waterPHPin, 
  waterTempPin, 
  pHCalibration, 
  pHReadingInterval, 
  pHReadingCount, 
  ADCResolution
  );

//Nextion HMI component for Waveform
NexWaveform soilPHWave = NexWaveform(1, 1, "s0"); 
NexWaveform soilMoistureWave = NexWaveform(2, 1, "s0"); 
NexWaveform nutritionWave = NexWaveform(3, 1, "s0"); 
NexWaveform waterPHWave = NexWaveform(4, 1, "s0"); 
NexWaveform waterTempWave = NexWaveform(5, 1, "s0"); 
NexWaveform waterLevelWave = NexWaveform(6, 1, "s0"); 
NexWaveform temperatureWave = NexWaveform(7, 1, "s0"); 
NexWaveform lightnessWave = NexWaveform(8, 1, "s0"); 
NexWaveform humidityWave = NexWaveform(9, 1, "s0"); 

//Nextion HMI component for Control Button
NexButton waterControlOn = NexButton(10, 1, "b0"); 
NexButton waterControlOff = NexButton(10, 2, "b1");
NexButton hydroponicsControlOn = NexButton(10, 3, "b2");
NexButton hydroponicsControlOff = NexButton(10, 4, "b3");
NexButton nutritionControlOn = NexButton(10, 5, "b4");
NexButton nutritionControlOff = NexButton(10, 6, "b5");
NexButton humidityControlOn = NexButton(10, 7, "b6");
NexButton humidityControlOff = NexButton(10, 8, "b7");

NexButton openLoopMode = NexButton(10, 10, "b9");
NexButton closeLoopMode = NexButton(10, 11, "b10");

NexTouch *nex_listen_list[] = 
{
  &waterControlOn,
  &waterControlOff,
  &hydroponicsControlOn,
  &hydroponicsControlOff,
  &nutritionControlOn,
  &nutritionControlOff,
  &humidityControlOn,
  &humidityControlOff,
  NULL
};

NexTouch *changeModeList[] =
{
  &openLoopMode,
  &closeLoopMode,
  NULL
};

//Monitoring Variables
float soilMoisture;
float waterLevel;
float temperature;
float humidity;
float waterPH;
float waterTemp;
float soilPH;
float tdsValue;
float readLux;

bool operationMode = 0;
int readSoil;
bool waterState = true;
int readPH;
int Distance = 0;
String sendToESP = "";

//HMI function for each button
void waterSetOn(void *ptr)
{
  digitalWrite(waterValvePin, HIGH);
  digitalWrite(relayPump2Pin, HIGH);
}

void waterSetOff(void *ptr)
{
  digitalWrite(relayPump2Pin, LOW);
  digitalWrite(waterValvePin, LOW);
}

void hydroponicsSetOn(void *ptr)
{
  digitalWrite(relayPump1Pin, HIGH);
}

void hydroponicsSetOff(void *ptr)
{
  digitalWrite(relayPump1Pin, LOW);
}

void nutritionSetOn(void *ptr)
{
  digitalWrite(nutritionValvePin, HIGH);
  digitalWrite(relayPump2Pin, HIGH); 
}

void nutritionSetOff(void *ptr)
{
  digitalWrite(relayPump2Pin, LOW);
  digitalWrite(nutritionValvePin, LOW);
}

void humiditySetOn(void *ptr)
{
  digitalWrite(relayMistPin, HIGH); 
}

void humiditySetOff(void *ptr)
{
  digitalWrite(relayMistPin, LOW); 
}

void setOpenLoopMode(void *ptr)
{
  operationMode = 1;
}

void setCloseLoopMode(void *ptr)
{
  operationMode = 0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial3.begin(115200);
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

  //Nextion initialization
  waterControlOn.attachPush(waterSetOn);
  waterControlOff.attachPush(waterSetOff);
  hydroponicsControlOn.attachPush(hydroponicsSetOn);
  hydroponicsControlOff.attachPush(hydroponicsSetOff);
  nutritionControlOn.attachPush(nutritionSetOn);
  nutritionControlOff.attachPush(nutritionSetOff);
  humidityControlOn.attachPush(humiditySetOn);
  humidityControlOff.attachPush(humiditySetOff);
  openLoopMode.attachPush(setOpenLoopMode);
  closeLoopMode.attachPush(setCloseLoopMode);
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
  Serial.print("soilPH.x0.val=");
  Serial.print(soilPH);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(soilPH < 6.5)
  {
    Serial.print("soilPH.t1.txt=");
    Serial.print("Acidic");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(soilPH > 7.5)
  {
    Serial.print("soilPH.t1.txt=");
    Serial.print("Alkaline");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("soilPH.t1.txt=");
    Serial.print("Neutral");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  soilPHWave.addValue(0, readPH);

  //WaterPump for Hydroponics Set-Up use Soil Moist sensor
  readSoil = analogRead(soilPin);
  soilMoisture = map(readSoil, 0, 1023, 100, 0);
  Serial.print("mainPage.x1.val=");
  Serial.print(soilMoisture);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("soilMoist.x0.val=");
  Serial.print(soilMoisture);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(soilMoisture < 50)
  {
    Serial.print("soilMoist.t1.txt=");
    Serial.print("Dry");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(soilMoisture > 70)
  {
    Serial.print("soilMoist.t1.txt=");
    Serial.print("Too Wet");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("soilMoist.t1.txt=");
    Serial.print("Hydrated");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  soilMoistureWave.addValue(0, soilMoisture);

  //Monitoring myTDS and Setting up the Nutrition use TDS sensor
  myTDS.setTemperature(waterTemp); //or use "temperature" from DHT22
  myTDS.update();
  tdsValue = myTDS.getTdsValue();
  Serial.print("mainPage.x2.val=");
  Serial.print(tdsValue);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("nutritionTDS.x0.val=");
  Serial.print(tdsValue);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(tdsValue < 800)
  {
    Serial.print("nutritionTDS.t1.txt=");
    Serial.print("Low");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(tdsValue > 1500)
  {
    Serial.print("nutritionTDS.t1.txt=");
    Serial.print("Over");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("nutritionTDS.t1.txt=");
    Serial.print("Optimum");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  nutritionWave.addValue(0, tdsValue);

  //Monitoring pH and Temp of Water and Nutrition use PH4502C
  waterPH = PH4502C.read_ph_level();
  Serial.print("mainPage.x3.val=");
  Serial.print(waterPH);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("waterPH.x0.val=");
  Serial.print(waterPH);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(waterPH < 6.5)
  {
    Serial.print("waterPH.t1.txt=");
    Serial.print("Acidic");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(waterPH > 7.5)
  {
    Serial.print("waterPH.t1.txt=");
    Serial.print("Alkaline");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("waterPH.t1.txt=");
    Serial.print("Neutral");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  waterPHWave.addValue(0, waterPH);

  waterTemp = PH4502C.read_temp();
  Serial.print("mainPage.x4.val=");
  Serial.print(waterTemp);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("waterTemp.x0.val=");
  Serial.print(waterTemp);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(waterTemp < 20)
  {
    Serial.print("waterTemp.t1.txt=");
    Serial.print("Cool");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(waterTemp > 40)
  {
    Serial.print("waterTemp.t1.txt=");
    Serial.print("Hot");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("waterTemp.t1.txt=");
    Serial.print("Normal");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  waterTempWave.addValue(0, waterTemp);
  
  //WaterPump for Tank Set-Up use Ultrasonic sensor
  Distance = getDistance();
  waterLevel = map(Distance, 20, 5, 0, 100);
  Serial.print("mainPage.x5.val=");
  Serial.print(waterLevel);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("waterLevel.x0.val=");
  Serial.print(waterLevel);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(waterLevel < 20)
  {
    Serial.print("waterLevel.t1.txt=");
    Serial.print("Low");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(waterLevel > 80)
  {
    Serial.print("waterlevel.t1.txt=");
    Serial.print("High");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("waterLevel.t1.txt=");
    Serial.print("Moderate");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  waterLevelWave.addValue(0, waterLevel);

  //MistMaker and DHT Set-Up use DHT22 sensor
  temperature = myDHT.readTemperature();
  Serial.print("mainPage.x6.val=");
  Serial.print(temperature);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("temperature.x0.val=");
  Serial.print(temperature);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(temperature < 20)
  {
    Serial.print("temperature.t1.txt=");
    Serial.print("Cool");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(temperature > 40)
  {
    Serial.print("temperature.t1.txt=");
    Serial.print("Hot");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("temperature.t1.txt=");
    Serial.print("Normal");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  temperatureWave.addValue(0, temperature);

  //Monitoring Light Intensity use BH1750
  readLux = luxMeter.readLightLevel();
  Serial.print("mainPage.x7.val=");
  Serial.print(readLux);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("lightness.x0.val=");
  Serial.print(readLux);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(readLux < 10000)
  {
    Serial.print("lightness.t1.txt=");
    Serial.print("Low");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(readLux > 30000)
  {
    Serial.print("lightness.t1.txt=");
    Serial.print("High");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("lightness.t1.txt=");
    Serial.print("Optimum");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  lightnessWave.addValue(0, readLux);

  humidity = myDHT.readHumidity();
  Serial.print("mainPage.x8.val=");
  Serial.print(humidity);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("humidity.x0.val=");
  Serial.print(humidity);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if(humidity < 60)
  {
    Serial.print("humidity.t1.txt=");
    Serial.print("Dry");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if(humidity > 80)
  {
    Serial.print("humidity.t1.txt=");
    Serial.print("Too Humid");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else
  {
    Serial.print("humidity.t1.txt=");
    Serial.print("Optimum");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  humidityWave.addValue(0, humidity);

  //send data to ESP
  sendToESP = "";
  sendToESP += soilPH;
  sendToESP += ";";
  sendToESP += soilMoisture;
  sendToESP += ";";
  sendToESP += tdsValue;
  sendToESP += ";";
  sendToESP += waterPH;
  sendToESP += ";";
  sendToESP += waterTemp;
  sendToESP += ";";
  sendToESP += waterLevel;
  sendToESP + ";";
  sendToESP += temperature;
  sendToESP += ";";
  sendToESP += readLux;
  sendToESP += ";";
  sendToESP += humidity ;
  Serial3.println(sendToESP);

  switch(operationMode)
  {
    case 1:
    nexLoop(nex_listen_list);
    break;

    default:
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
  if (soilMoisture <= 70)
    {
      digitalWrite(relayPump1Pin, HIGH);
    }
  else
    {
      digitalWrite(relayPump1Pin, LOW);
    }
    
  }

  nexLoop(changeModeList);
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
