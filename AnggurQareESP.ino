#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

const char *ssid = "ITS-WIFI1-2G";  
const char *password = "itssurabaya";

float soilPH, readLux, waterPH, waterTemp, waterLevel, tdsValue, temperature, humidity, soilPercentage;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connectToWifi();
}

void connectToWifi(){
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //This line hides the viewing of ESP as wifi hotspot
  
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

String splitString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    String msg = "";
    while(Serial.available()){
      msg += char(Serial.read());
      delay(50);
    }
    soilPH = splitString(msg, ';', 0).toFloat();
    readLux = splitString(msg, ';', 1).toFloat();
    waterPH = splitString(msg, ';', 2).toFloat();
    waterTemp = splitString(msg, ';', 3).toFloat();
    waterLevel = splitString(msg, ';', 4).toFloat();
    tdsValue = splitString(msg, ';', 5).toFloat();
    temperature = splitString(msg, ';', 6).toFloat();
    humidity = splitString(msg, ';', 7).toFloat();
    soilPercentage = splitString(msg, ';', 8).toFloat();
    sendDataToServer();
//    Serial.print(msg);
  }
}

void sendDataToServer(){
  WiFiClient client;
  HTTPClient http;    //Declare object of class HTTPClient
  String postData;
  //Post Data
  postData = "soil_ph=";
  postData += soilPH;
  postData += "&lightness=";
  postData += readLux;
  postData += "&water_ph=";
  postData += waterPH;
  postData += "&water_temp=";
  postData += waterTemp;
  postData += "&water_level=";
  postData += waterLevel;
  postData += "&nutrition_tds=";
  postData += tdsValue;
  postData += "&temperature=";
  postData += temperature;
  postData += "&humidity=";
  postData += humidity;
  postData += "&soil_moisture=";
  postData += soilPercentage;
  
  http.begin(client, "http://10.8.108.42/AnggurQare_v1.0/server.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
  int httpCode = http.POST(postData);   //Send the request
  String payload = http.getString();    //Get the response payload
      
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
      
  http.end();
}
