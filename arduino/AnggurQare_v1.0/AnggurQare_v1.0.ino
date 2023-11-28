#include <Wire.h>
#include <SPI.h>
#include "Nextion.h"
#include "BH1750.h"
#include "GravityTDS.h"
#include "DHT.h"
#include "ph4502c_sensor.h"

#define soilPin A0
#define pHPin A1
#define tdsPin A2
#define waterPHPin A3
#define waterTempPin A4
#define relayPump1Pin 2
#define relayPump2Pin 7
#define relayMistPin 8
#define waterValvePin 3
#define nutritionValvePin 4
#define echoPin 5
#define triggerPin 6
#define dhtPin 9
#define maxDistance 200
//#define pHTriggerPin 12
//#define pHCalibration 14.8f
//#define pHReadingInterval 100
//#define pHReadingCount 10
//#define ADCResolution 1024

//-----------------UPTDATE WATER PH CODE (WILL UPDATE SOON)---------------------//
/*  #define ph_Pin  A3
float Po = 0;
float pH_step;
int nilai_analog_pH;
double TeganganPh;

float myPH4 = 4.200;
float myPH7 = 3.320; 

.................................................................

  nilai_analog_pH = analogRead(ph_Pin); -> PENTING
  Serial.print("ADC : ");
  Serial.println(nilai_analog_pH);
  TeganganPh = nilai_analog_pH * (5.0/1024.0); -> PENTING
  Serial.print("Tegangan : ");
  Serial.println(TeganganPh, 3);

  pH_step = (myPH4 - myPH7) /3; -> PENTING
  Po = 7.00 + ((myPH7 - TeganganPh) / pH_step); -> PENTING
  Serial.print("Nilai PH: ");
  Serial.println(Po, 2);
  delay(1000);

------------------------------------------------------------------

*/

DHT myDHT(dhtPin, DHT22);
GravityTDS myTDS;
BH1750 luxMeter(0x23); //or change to 0x5C if trouble
PH4502C_Sensor PH4502C(
  waterPHPin, 
  waterTempPin
  );

//---------- Nextion HMI component for Waveform ----------//
NexWaveform soilPHWave = NexWaveform(1, 1, "s0"); 
NexWaveform soilMoistureWave = NexWaveform(2, 1, "s0"); 
NexWaveform nutritionWave = NexWaveform(3, 1, "s0"); 
NexWaveform waterPHWave = NexWaveform(4, 1, "s0"); 
NexWaveform waterTempWave = NexWaveform(5, 1, "s0"); 
NexWaveform waterLevelWave = NexWaveform(6, 1, "s0"); 
NexWaveform temperatureWave = NexWaveform(7, 1, "s0"); 
NexWaveform lightnessWave = NexWaveform(8, 1, "s0"); 
NexWaveform humidityWave = NexWaveform(9, 1, "s0"); 

//---------- Nextion HMI component for Control Button ----------//
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

//---------- Monitoring Variables ----------//
float soilMoisture;
float waterLevel;
float temperature;
float humidity;
float waterPH;
float waterTemp;
float waterTemp_act; // deviding waterTemp by 10 to get real temp data
float soilPH;
float tdsValue;
float readLux;

float pH_out = 0;
float pH_step;
int nilai_analog_pH;
double Tegangan_pH;
float myPH4 = 4.200; //calibrating voltage when ph 4
float myPH7 = 3.320; //calibrating voltage when ph 7

bool operationMode = 0;
int readSoil;
bool waterState = true;
int readPH;
int Distance = 0;
String sendToESP = "";

//---------- HMI function for each button ----------//
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

//---------- Ultrasonic Function ----------//
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
    //Serial1.print("distance=");Serial1.println(distance);

    // Catch funky ∞ distance readings
    watchloop++;        
    if (watchloop > 20){      // If errant "676" readings 20 times
      distance = 610;         // set distance to 610cm (20ft) 
      break;                  // and break out of loop (not really needed if forced to 610)
    }
    
  }

  return (distance);
}

//---------- GET AND SEND SOIL PH DATA ----------//
void readPH_function()
{
   //Monitoring Soil pH use Soil pH sensor
  readPH = analogRead(pHPin);
  soilPH = (-0.0693 * readPH) + 7.3855;
  Serial1.print("mainPage.x0.val=");
  Serial1.print(soilPH);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("soilPH.x0.val=");
  Serial1.print(soilPH);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(soilPH < 6.5)
  {
    Serial1.print("soilPH.t1.txt=");
    Serial1.print("Acidic");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(soilPH > 7.5)
  {
    Serial1.print("soilPH.t1.txt=");
    Serial1.print("Alkaline");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("soilPH.t1.txt=");
    Serial1.print("Neutral");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  soilPHWave.addValue(0, readPH);
}

//---------- GET AND SEND SOIL MOISTURE DATA ----------//
void soilMoisture_function()
{
  //WaterPump for Hydroponics Set-Up use Soil Moist sensor
  readSoil = analogRead(soilPin);
  soilMoisture = map(readSoil, 0, 1023, 100, 0);
  Serial1.print("mainPage.x1.val=");
  Serial1.print(soilMoisture);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("soilMoist.x0.val=");
  Serial1.print(soilMoisture);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(soilMoisture < 50)
  {
    Serial1.print("soilMoist.t1.txt=");
    Serial1.print("Dry");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(soilMoisture > 70)
  {
    Serial1.print("soilMoist.t1.txt=");
    Serial1.print("Too Wet");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("soilMoist.t1.txt=");
    Serial1.print("Hydrated");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  soilMoistureWave.addValue(0, soilMoisture);
}

//---------- GET AND SEND NUTRITION DATA ----------//
void tdsValue_function()
{
  //Monitoring myTDS and Setting up the Nutrition use TDS sensor
  myTDS.setTemperature(25); //or use "temperature" from DHT22
  myTDS.update();
  tdsValue = myTDS.getTdsValue();
  Serial1.print("mainPage.x2.val=");
  Serial1.print(tdsValue);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("nutritionTDS.x0.val=");
  Serial1.print(tdsValue);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(tdsValue < 800)
  {
    Serial1.print("nutritionTDS.t1.txt=");
    Serial1.print("Low");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(tdsValue > 1500)
  {
    Serial1.print("nutritionTDS.t1.txt=");
    Serial1.print("Over");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("nutritionTDS.t1.txt=");
    Serial1.print("Optimum");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  nutritionWave.addValue(0, tdsValue);
}

//---------- GET AND SEND WATER PH DATA ----------//
void waterPH_function()
{

  nilai_analog_pH = analogRead(waterPHPin); //-> PENTING
  Tegangan_pH = nilai_analog_pH * (5.0/1024.0); //-> PENTING
  pH_step = (myPH4 - myPH7) /3; //-> PENTING
  waterPH = 7.00 + ((myPH7 - Tegangan_pH) / pH_step); //-> PENTING
  //Monitoring pH and Temp of Water and Nutrition use PH4502C
  //waterPH = PH4502C.read_ph_level();
  Serial1.print("mainPage.x3.val=");
  Serial1.print(waterPH);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("waterPH.x0.val=");
  Serial1.print(waterPH);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(waterPH < 6.5)
  {
    Serial1.print("waterPH.t1.txt=");
    Serial1.print("Acidic");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(waterPH > 7.5)
  {
    Serial1.print("waterPH.t1.txt=");
    Serial1.print("Alkaline");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("waterPH.t1.txt=");
    Serial1.print("Neutral");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  waterPHWave.addValue(0, waterPH);
}

//---------- GET AND SEND WATER TEMPERATURE DATA ----------//
void waterTemp_function()
{
  waterTemp = PH4502C.read_temp();
  waterTemp_act = waterTemp / 10.0;
  Serial1.print("mainPage.x4.val=");
  Serial1.print(waterTemp_act);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("waterTemp.x0.val=");
  Serial1.print(waterTemp_act);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(waterTemp_act < 20)
  {
    Serial1.print("waterTemp.t1.txt=");
    Serial1.print("Cool");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(waterTemp_act > 40)
  {
    Serial1.print("waterTemp.t1.txt=");
    Serial1.print("Hot");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("waterTemp.t1.txt=");
    Serial1.print("Normal");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  waterTempWave.addValue(0, waterTemp_act);
}

//---------- GET AND SEND WATER LEVEL DATA ----------//
void waterLevel_function()
{
  //WaterPump for Tank Set-Up use Ultrasonic sensor
  Distance = getDistance();
  waterLevel = map(Distance, 20, 5, 0, 100);
  Serial1.print("mainPage.x5.val=");
  Serial1.print(waterLevel);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("waterLevel.x0.val=");
  Serial1.print(waterLevel);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(waterLevel < 20)
  {
    Serial1.print("waterLevel.t1.txt=");
    Serial1.print("Low");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(waterLevel > 80)
  {
    Serial1.print("waterlevel.t1.txt=");
    Serial1.print("High");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("waterLevel.t1.txt=");
    Serial1.print("Moderate");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  waterLevelWave.addValue(0, waterLevel);
}

//---------- GET AND SEND TEMPERATURE DATA ----------//
void temperature_function()
{
  //MistMaker and DHT Set-Up use DHT22 sensor
  temperature = myDHT.readTemperature();
  Serial1.print("mainPage.x6.val=");
  Serial1.print(temperature);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("temperature.x0.val=");
  Serial1.print(temperature);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(temperature < 20)
  {
    Serial1.print("temperature.t1.txt=");
    Serial1.print("Cool");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(temperature > 40)
  {
    Serial1.print("temperature.t1.txt=");
    Serial1.print("Hot");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("temperature.t1.txt=");
    Serial1.print("Normal");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  temperatureWave.addValue(0, temperature);
}

//---------- GET AND SEND LIGHTNESS DATA ----------//
void readLux_function()
{
  //Monitoring Light Intensity use BH1750
  readLux = luxMeter.readLightLevel();
  Serial1.print("mainPage.x7.val=");
  Serial1.print(readLux);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("lightness.x0.val=");
  Serial1.print(readLux);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(readLux < 10000)
  {
    Serial1.print("lightness.t1.txt=");
    Serial1.print("Low");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(readLux > 30000)
  {
    Serial1.print("lightness.t1.txt=");
    Serial1.print("High");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("lightness.t1.txt=");
    Serial1.print("Optimum");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  lightnessWave.addValue(0, readLux);
}

//---------- GET AND SEND HUMIDITY DATA ----------//
void humidity_function()
{
  humidity = myDHT.readHumidity();
  Serial1.print("mainPage.x8.val=");
  Serial1.print(humidity);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.print("humidity.x0.val=");
  Serial1.print(humidity);
  Serial1.write(0xff);
  Serial1.write(0xff);
  Serial1.write(0xff);
  if(humidity < 60)
  {
    Serial1.print("humidity.t1.txt=");
    Serial1.print("Dry");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else if(humidity > 80)
  {
    Serial1.print("humidity.t1.txt=");
    Serial1.print("Too Humid");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  else
  {
    Serial1.print("humidity.t1.txt=");
    Serial1.print("Optimum");
    Serial1.write(0xff);
    Serial1.write(0xff);
    Serial1.write(0xff);
  }
  humidityWave.addValue(0, humidity);
}

//---------- send data to ESP -----------//
void sendToESP_function()
{
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
  sendToESP += humidity;
  Serial3.println(sendToESP);
  
  if(Serial3.available())
  {
   String msg = "";
   while(Serial3.available())
   {
    msg += char(Serial3.read());
    delay(50);
   }

  Serial.println(msg);
  }
  
  delay(1000);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial3.begin(115200);
  nexInit();
  myDHT.begin();
  Wire.begin();
  luxMeter.begin();
  Serial1.println("Machine ON");
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
  readPH_function();
  delay(50);
  soilMoisture_function();
  delay(50);
  tdsValue_function();
  delay(50);
  waterPH_function();
  delay(50);
  waterTemp_function();
  delay(50);
  waterLevel_function();
  delay(50);
  temperature_function();
  delay(50);
  readLux_function();
  delay(50);
  humidity_function();
  delay(50);
  sendToESP_function();
  delay(50); 

//---------- MAIN LOGIC FOR OPERATING ----------//

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
      delay(100);

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
      delay(100);
      
    }

    //Logic for Refill Nutrition 
  if (waterLevel == 100 && tdsValue < 1000 && waterState == false)
    {
      digitalWrite(nutritionValvePin, HIGH);
      digitalWrite(relayPump2Pin, HIGH);
      delay(100);

    }
  else
    {
      digitalWrite(relayPump2Pin, LOW);
      digitalWrite(nutritionValvePin, LOW);
      delay(100);

    }
  
  //Logic for Mist Maker to humid the environment
  if (humidity < 70 && tdsValue > 900 && waterState == false)
    {
      digitalWrite(relayMistPin, HIGH);
      delay(100);

    }
  else
    {
      digitalWrite(relayMistPin, LOW);
      delay(100);

    }
  
  //Logic for Watering the Hydroponics
  if (soilMoisture <= 70)
    {
      digitalWrite(relayPump1Pin, HIGH);
      delay(100);

    }
  else
    {
      digitalWrite(relayPump1Pin, LOW);
      delay(100);

    }
    
  }

  nexLoop(changeModeList);
}