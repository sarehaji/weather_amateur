#include <ArduinoJson.h>
#include <Adafruit_BMP085.h>
#include <SoftwareSerial.h>
SoftwareSerial linkSerial(3, 1);

Adafruit_BMP085 bmp;

int arahangin = 1;
unsigned long previousMillis = 0;
int delaytime = 10000;
float pressure = 0;

//Anemometer
#define windPin 14

const float pi = 3.14159265; // pi number
int period = 10000; // Measurement period (miliseconds)
int radio = 90; // Distance from center windmill to outer cup (mm)
int jml_celah = 18; // jumlah celah sensor
unsigned int Sample = 0; // Sample number
unsigned int counter = 0; // B/W counter for sensor
unsigned int RPM = 0; // Revolutions per minute
float speedwind = 0; // Wind speed (m/s)

void RPMcalc()
{
  RPM=((counter/jml_celah)*60)/(period/1000); // Calculate revolutions per minute (RPM)
  //linkSerial.print("RPM: ");
  //linkSerial.print(RPM);
}

void WindSpeed()
{
  speedwind = ((2 * pi * radio * RPM)/60) / 1000; // Calculate wind speed on m/s
  //linkSerial.print("; Wind speed: ");
  //linkSerial.print(speedwind);
  //linkSerial.print(" [m/s]");
  //linkSerial.println();
  counter = 0;
}

ICACHE_RAM_ATTR void addcount()
{
  counter++;
}

//SendData
void sendWind(){
  StaticJsonDocument<200> doc;
  doc["wind"] = speedwind;
  doc["pressure"] = pressure;
  
  serializeJson(doc, linkSerial);
  linkSerial.println("Sending...");
}

void readBMP(){
    //linkSerial.print("Pressure = ");
    pressure = bmp.readPressure();
    //linkSerial.print(pressure);
    //linkSerial.println(" Pa");
}

void sendWindNoJson(){
  String windString = String(speedwind);
  String pressureString = String(pressure);
  String full = windString + "<" + pressureString;
  char* buf = (char*) malloc(sizeof(char)*13);
  full.toCharArray(buf, 13);
  
  linkSerial.write(buf);
  free(buf);
}

//Main
void setup() {
  
  pinMode(windPin, INPUT);
  digitalWrite(windPin, HIGH);

  linkSerial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(windPin), addcount, CHANGE);
  if (!bmp.begin()) {
    linkSerial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delaytime) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    RPMcalc();
    WindSpeed();
    readBMP();
    //sendWind();
    sendWindNoJson();
  }
}
