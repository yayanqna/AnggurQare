#include "DHT.h"
#include "Wire.h"
#include "ModbusRtu.h"
#include "SPI.h"

#define soilPin A0
#define relayPump1 2
#define relayPump2 7
#define relayMist 8
#define echoPin 5
#define triggerPin 6
#define dhtPin 9
#define maxDistance 200

DHT myDHT(dhtPin, DHT22);

int soilPercentage;
int readSoil;
int waterLevel;
float temperature;
float humidity;
bool waterState = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myDHT.begin();
  Serial.println("Machine ON");
  pinMode(relayPump1, OUTPUT);
  pinMode(relayPump2, OUTPUT);
  pinMode(relayMist, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT); //or use INPUT_PULLUP if trouble
  digitalWrite(triggerPin, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  //WaterPump for Tank Set-Up
  int Distance = 0;
  Distance = getDistance();
  waterLevel = map(Distance, 20, 5, 0, 100);
  if (waterLevel < 100 && waterState == true)
    {
      digitalWrite(relayPump2, HIGH);
    }
  else
    {
      digitalWrite(relayPump2, LOW);
      waterState = false;

      if (waterLevel <= 10)
        {
          waterState = true;
        }
    }
  
  //MistMaker and DHT Set-Up
  temperature = myDHT.readTemperature();
  humidity = myDHT.readHumidity();
  if (humidity <= 70)
    {
      digitalWrite(relayMist, HIGH);
    }
  else
    {
      digitalWrite(relayMist, LOW);
    }
  
  //WaterPump for Hydroponics Set-Up
  readSoil = analogRead(soilPin);
  soilPercentage = map(readSoil, 0, 1023, 0, 100);
  if (soilPercentage <= 70)
    {
      digitalWrite(relayPump1, HIGH);
    }
  else
    {
      digitalWrite(relayPump1, LOW);
    }
}


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
