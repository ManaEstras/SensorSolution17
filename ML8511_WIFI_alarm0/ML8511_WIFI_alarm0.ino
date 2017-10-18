#include <WiFi101.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <BlynkSimpleMKR1000.h>
#include <SimpleTimer.h>


// ARTIK Cloud REST endpoint
char server[] = "api.artik.cloud";  
int port = 443; // We're using HTTPS

// Device ID tokens
String deviceToken = "5d8b59d5a7f343d3a150c605a88a1191";
String deviceId = "f40c91c50ffe44c48f37a6ea20afae82";
// Your wifi network
char ssid[] = "SEU-WLAN-2.4";     
char pass[] = "madezhizhang";
const int ledPin =  2;

char auth[] = "ad48388f565e44cf8b674f2272ebf0af";

float uvIntensity = 0.0;
boolean onFire = false;
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
int X;
int Y;

 
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

char buf[200]; // buffer to store the JSON to be sent to the ARTIK cloud

const int LED = 6;
int ledState = 0;
int measurePin = A1; //Connect dust sensor to Arduino A0 pin
int ledPower = 1;   //Connect 3 led driver pins of dust sensor to Arduino D2

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, server, port);

int status = WL_IDLE_STATUS;

BLYNK_WRITE(V3)
{   
  X = param.asInt(); // Get value as integer
}

BLYNK_WRITE(V4)
{   
  Y = param.asInt(); // Get value as integer
}

void setup() {
pinMode(ledPin, OUTPUT);
  pinMode(LED,OUTPUT);
  pinMode(ledPower,OUTPUT);

  Serial.begin(9600);
  while ( status != WL_CONNECTED) { // Keep trying until connected
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                  
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }
  Blynk.begin(auth, ssid, pass);
}
  
void loop() {
  digitalWrite(ledPin, HIGH);
  Serial.println("loop");
  ledToggle();
  float acu=0; 
  float timelim=0;
  float totaltime=0;
  float intensitylim=0.5;
  int interval=0;
  Serial.println("making POST request");
  String contentType = "application/json";
  String AuthorizationData = "Bearer " + deviceToken; //Device Token

  float uvLevel = averageAnalogRead(A0) * 3.3 / 1024;
  
  float uvIntensity = mapfloat(uvLevel, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level
  onFire = false;

  Serial.print(" / ML8511 voltage: ");
  Serial.print(uvLevel);

  Serial.print(" / UV Intensity (mW/cm^2): ");
  Serial.print(uvIntensity);
  Serial.print("acu ");
  Serial.print(acu);

  while(uvIntensity>0.4||interval>1){
    
  ledToggle();
  
  Serial.println("The UV is too strong");


  uvLevel = averageAnalogRead(A0) * 3.3 / 1024;
  
  uvIntensity = mapfloat(uvLevel, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level
 
  timelim=-12.9736*uvIntensity*uvIntensity*uvIntensity+85.2522*uvIntensity*uvIntensity-190.9085*uvIntensity+175.7416;
  acu = acu + 5 * 1 / timelim;
  onFire = false;

  Serial.print(" / ML8511 voltage: ");
  Serial.print(uvLevel);

  Serial.print(" / UV Intensity (mW/cm^2): ");
  Serial.print(uvIntensity);
  Serial.print("acupercent ");
  Serial.print(acu);

  if (uvIntensity > intensitylim && acu > 0.5 || uvIntensity > intensitylim*3) {   //Testing 
    // turn LED on:
     interval=5;
    digitalWrite(ledPin, LOW);
    Blynk.notify("You have been exposed to too much UV. Go get indoor");
    Blynk.setProperty(V2,"color","#D3435C");
    }

    
  if (uvIntensity<intensitylim&&acu>0.5)
  {
    
    interval=interval-1; 
    Blynk.setProperty(V2,"color","#5F7CD8");
  }
  
    //delay(1000); // delay 10 sec
  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
 
  voMeasured = averageAnalogRead(measurePin); // read the dust value
 
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
 
  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (3.3 / 1024.0);
 
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;

  Serial.print("'\n'");
  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
 
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage);
 
  Serial.print(" - Dust Density: ");
  Serial.println(dustDensity); // unit: mg/m3

  //****************Dust Sensor Collecting
  
  int len = loadBuffer(uvIntensity,dustDensity,acu);  
  Serial.println("   Sending data "+String(uvIntensity)+"   "+String(dustDensity)); 
  
  // push the data to the ARTIK Cloud
  client.beginRequest();
  client.post("/v1.1/messages"); //, contentType, buf
  client.sendHeader("Authorization", AuthorizationData);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", len);
  client.endRequest();
  client.print(buf);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  
  Serial.print("Response: ");
  Serial.println(response);

  Serial.println("Wait a bit");

    Serial.println("Wait a bit");

  Blynk.run();
  Blynk.virtualWrite(V0, uvIntensity);
    // This command writes p to Virtual Pin (0)
  Blynk.virtualWrite(V1, dustDensity);
  Blynk.virtualWrite(V2, acu);
  Serial.print("'\n'");
  Serial.print("Blynk Sending");
  Blynk.run();
  BLYNK_WRITE(V3);
  BLYNK_WRITE(V4);
  Serial.print("'\n'");
  Serial.print("Receiveing Blynk    ");
  Serial.print(X);
  Serial.print("'\n'");
  Serial.print("Receiveing Blynk    ");
  Serial.print(Y);

 
  delay(300); // delay 10 sec

}



  

  //***************UV sensor collecting

  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
 
  voMeasured = averageAnalogRead(measurePin); // read the dust value
 
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
 
  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (3.3 / 1024.0);
 
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;

  Serial.print("'\n'");
  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
 
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage);
 
  Serial.print(" - Dust Density: ");
  Serial.println(dustDensity); // unit: mg/m3

  //****************Dust Sensor Collecting
  
  int len = loadBuffer(uvIntensity,dustDensity,acu);  
  Serial.println("   Sending data "+String(uvIntensity)+"   "+String(dustDensity)); 
  
  // push the data to the ARTIK Cloud
  client.beginRequest();
  client.post("/v1.1/messages"); //, contentType, buf
  client.sendHeader("Authorization", AuthorizationData);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", len);
  client.endRequest();
  client.print(buf);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  
  Serial.print("Response: ");
  Serial.println(response);

  Serial.println("Wait a bit");

  Blynk.run();
  Serial.print("Blynk running");
  Serial.print("'\n'");
  Blynk.virtualWrite(V0, uvIntensity);
    // This command writes p to Virtual Pin (0)
  Blynk.virtualWrite(V1, dustDensity);
  Blynk.virtualWrite(V2, acu);
  Serial.print("'\n'");
  Serial.print("Blynk Sending");


  Blynk.run();
  Serial.print("Blynk running");
  Serial.print("'\n'");
  BLYNK_WRITE(V3);
  BLYNK_WRITE(V4);
  Serial.print("'\n'");
  Serial.print("Receiveing Blynk    ");
  Serial.print(X);
  Serial.print("'\n'");
  Serial.print("Receiveing Blynk    ");
  Serial.print(Y);



 
  delay(300); // delay 10 sec

}

int loadBuffer(float uvIn, float dust, float acum ) {  
   StaticJsonBuffer<200> jsonBuffer; // reserve spot in memory

   JsonObject& root = jsonBuffer.createObject(); // create root objects
     root["sdid"] =  deviceId;  
     root["type"] = "message";

   JsonObject& dataPair = root.createNestedObject("data"); // create nested objects
     dataPair["dustDensity"] = dust;  
     dataPair["uvIntensity"] = uvIn;
     dataPair["accumulation"] = acum;

   root.printTo(buf, sizeof(buf)); // JSON-print to buffer

   return (root.measureLength()); // also return length
 }

 void ledToggle(){
  if (ledState == 0){
    digitalWrite(LED,LOW);
    ledState = 1;
  } else {
    digitalWrite(LED,HIGH);
    ledState = 0;
  }
 }

 //The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}
