
#include <WiFiManager.h>
#include <WiFi.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <ArduinoJson.h>
#include <AWS_IOT.h>
#include <EEPROM.h>

SoftwareSerial ss(16, 17);
SoftwareSerial linkSerial(3, 1);

AWS_IOT aws;
#define CLIENT_ID "ISATU"// thing unique ID, this id should be unique among all things associated with your AWS account.
#define MQTT_TOPIC "/outTopic" //topic for the MQTT data
#define AWS_HOST "a2sjer8nql5p21-ats.iot.ap-southeast-1.amazonaws.com" // your host for uploading data to AWS,
#define BUFFER_LEN 256
char msg[BUFFER_LEN];

#include <DHT.h>
#define dhtPin 2
#define dhtType DHT22
float humidity = 0;
float temperature = 0;
float pressure = 0;

DHT dht(dhtPin, dhtType);
TinyGPS gps;

#define pin_interrupt 13

#define utara 36
#define tl 39
#define timur 34
#define tenggara 35
#define selatan 32
#define bd 33
#define barat 25
#define bl 26

float milimeter_per_tip = 0.70;
long int jumlah_tip = 0;
float curah_hujan_per_menit = 0.00;
float curah_hujan_per_jam = 0.00;
float curahMenit = 0;
float curahJam = 0;
int counterMenit = 0;
int counterJam = 0;
bool flag = false;
static unsigned long last_interrupt_time = 0;

float lat = 0; 
float lon = 0;

float windSpeed = 0;
int arahangin = 1;
float last_temp = 0;
float last_hum = 0;

float arrayTemperature[60];
float arrayHumidity[60];
float arrayPressure[60];
float arrayWindSpeed[60];

char idAlatChar[34];

int systemCounter = 0;


//Wifi
void wifiStart(){
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // WiFi.mode(WiFi_STA); // it is a good practice to make sure your code sets wifi mode how you want it.
    WiFiManagerParameter custom_idAlat("idAlat", "idAlat", idAlatChar, 34);
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;
    wm.addParameter(&custom_idAlat);
    
    //run if want reset
//    wm.resetSettings();
//    for (int i = 0; i < 34; i++) {
//      EEPROM.write(i, 0);
//    }
//    EEPROM.commit();
    //run if want reset

    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("ISATU","stasiuncuaca"); // password protected ap

    if(!res) {
        linkSerial.println("Failed to connect");
        //ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        linkSerial.println("connected...yeey :)");
    }

    linkSerial.println(custom_idAlat.getValue());
//    if(SPIFFS.exists("/idAlat.txt")){
//      Serial.println("idAlat is still alive");
//      idAlat = readIDAlat("/idAlat.txt");
//    }
//    else{
//      Serial.println("There is no file idAlat.txt"); //read sensor
//      idAlat = atoi(custom_idAlat.getValue());
//      strcpy(idAlatChar, custom_idAlat.getValue());
//      saveFS(idAlatChar,"/idAlat.txt");
//    }
    if(EEPROM.read(0) == 0){
      linkSerial.println("Belum ada ID Alat");
      String convertString = custom_idAlat.getValue();
      
      for (int j = 0; j < 34; j++) { 
        EEPROM.write(j, convertString[j]);  
      }
      EEPROM.commit();
    }
    else{
      linkSerial.println("Sudah ada ID Alat");
    }
    
    linkSerial.println("START");
    //WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    //Serial.print("  ");
    linkSerial.println("\n  Connected.\n  Done");
    linkSerial.print("\n  Initializing DHT11...");
    linkSerial.println("  Done.");
    linkSerial.println("\n  Initializing connetction to AWS....");
    if(aws.connect(AWS_HOST, CLIENT_ID) == 0){ // connects to host and returns 0 upon success
      linkSerial.println("  Connected to AWS\n  Done.");
    }
    else {
      linkSerial.println("  Connection failed!\n make sure your subscription to MQTT in the test page");
    }
    linkSerial.println("  Done.\n\nDone.\n");
}

//WindVane
void WindVane(){
    if(digitalRead(utara)==LOW){arahangin = 1;}
    else if(digitalRead(tl)==LOW){arahangin = 2;}
    else if(digitalRead(timur)==LOW){arahangin = 3;}
    else if(digitalRead(tenggara)==LOW){arahangin = 4;}
    else if(digitalRead(selatan)==LOW){arahangin = 5;}
    else if(digitalRead(bd)==LOW){arahangin = 6;}
    else if(digitalRead(barat)==LOW){arahangin = 7;}
    else if(digitalRead(bl)==LOW){arahangin = 8;}
    linkSerial.print("Arah Angin = ");
    linkSerial.println(arahangin);
}

//Pressure
//void readBMP(){
//    linkSerial.print("Pressure = ");
//    pressure = bmp.readPressure();
//    linkSerial.print(pressure);
//    linkSerial.println(" Pa");
//    arrayPressure[systemCounter] = pressure;
//    
//}

//SendData
String idAlat;

void sendAWS(){
    float sumTemperature = 0;
    float sumHumidity = 0;
    float sumWindSpeed = 0;
    float sumPressure = 0;
    float avgTemperature = 0;
    float avgHumidity = 0;
    float avgWindSpeed = 0;
    float avgPressure = 0;
    
    for(int i=0; i<60; i++){
      sumTemperature = sumTemperature + arrayTemperature[i];
      sumHumidity = sumHumidity + arrayHumidity[i];
      sumWindSpeed = sumWindSpeed + arrayWindSpeed[i];
      sumPressure = sumPressure + arrayPressure[i];
    }

    avgTemperature = sumTemperature/60;
    avgHumidity = sumHumidity/60;
    avgWindSpeed = sumWindSpeed/60;
    avgPressure = sumPressure/60;

    for (int l = 0; l < 34; l++) { 
      idAlat += char(EEPROM.read(l)); 
    }
    
    String idString = String(idAlat);
    idAlat = "";

    if((isnan(avgTemperature)||isnan(avgHumidity))&&(last_temp == 0)||(last_hum == 0)){
      avgTemperature = 30;
      avgHumidity = 90;
    }
    else if((isnan(avgTemperature)||isnan(avgHumidity))){
      avgTemperature = last_temp;
      avgHumidity = last_hum;
    }
    else{
      last_temp = avgTemperature;
      last_hum = avgHumidity;
    }
    
    snprintf (msg, BUFFER_LEN, "{\"id_alat\" : \"%s\", \"temperature\" : %f, \"humidity\" : %f, \"wind_speed\" : %f, \"wind_direction\" : %d, \"latitude\" : %f, \"longitude\" : %f, \"rainfall\" : %f, \"pressure\" : %f}", idString.c_str(), avgTemperature, avgHumidity, avgWindSpeed, arahangin, lat, lon, curahJam, avgPressure );
    if(aws.publish(MQTT_TOPIC, msg) == 0){// publishes payload and returns 0 upon success
      linkSerial.println("Success\n");
    }
    else{
      linkSerial.println("Failed!\n");
      sendAWS();
    }
    curahJam = 0;
}

//Rainfall
ICACHE_RAM_ATTR void hitung_curah_hujan()
{
  unsigned long new_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (new_time - last_interrupt_time > 200) 
  {
    flag = true;
  }
  last_interrupt_time = new_time;
}

void readCurah(){
    counterMenit++;
    counterJam++;
    
    if(counterMenit == 6){
      curahMenit = curah_hujan_per_menit;
      curah_hujan_per_menit = 0;
      counterMenit = 0;
      linkSerial.print("Curah Hujan Per Menit = ");
      linkSerial.println(curahMenit);
    }
    if(counterJam == 6*60){
      curahJam = curah_hujan_per_jam;
      curah_hujan_per_jam = 0;
      counterJam = 0;
      linkSerial.print("Curah Hujan Per Jam = ");
      linkSerial.println(curahJam);
    }
}

//UARTSlave

void receiveUARTNoJson(){
//  char s[13];
  String s;
  if(linkSerial.available()){
    s = linkSerial.readString();
  }
  linkSerial.println(s);

//  for(int i = 0; i<13 ; i++){
//    if(linkSerial.available()>0){
//      s[i] = linkSerial.read();
//    }
//  }

  String windString = "";
  String pressureString = "";

  bool stateUART = false;
  for(int l = 0; l <s.length(); l++){
    if(stateUART == false && s[l] != '<'){
      windString += char(s[l]);
    }
    else if(s[l]=='<'){
      stateUART = true;
      continue;
    }
    else if(stateUART == true){
      pressureString += char(s[l]);
    }  
  }
  windSpeed = windString.toFloat();
  pressure = pressureString.toFloat();
  linkSerial.println("WindSpeed = ");
  linkSerial.println(windSpeed);
  linkSerial.println("Pressure = ");
  linkSerial.println(pressure);
  arrayWindSpeed[systemCounter] = windSpeed;
  arrayPressure[systemCounter] = pressure;
}

//void receiveUART(){
//  if (linkSerial.available()) 
//  {
//    // Allocate the JSON document
//    // This one must be bigger than for the sender because it must store the strings
//    StaticJsonDocument<300> doc;
//    // Read the JSON document from the "link" serial port
//    DeserializationError err = deserializeJson(doc, linkSerial);
//
//    if (err == DeserializationError::Ok) 
//    {
//      // Print the values
//      // (we must use as<T>() to resolve the ambiguity)
//      linkSerial.print("Wind = ");
//      linkSerial.println(doc["wind"].as<float>());
//      linkSerial.print("Pressure = ");
//      linkSerial.println(doc["pressure"].as<float>());
//      windSpeed = doc["wind"].as<float>();
//      pressure = doc["pressure"].as<float>();
//      arrayWindSpeed[systemCounter] = windSpeed;
//      arrayPressure[systemCounter] = pressure;
//      
//    } 
//    else 
//    {
//       //Print error to the "debug" serial port
//      linkSerial.print("deserializeJson() returned ");
//      linkSerial.println(err.c_str());
//  
//      // Flush all bytes in the "link" serial port buffer
//      while (linkSerial.available() > 0)
//        linkSerial.read();
//    }
////    linkSerial.print("RPM = 180");
////    linkSerial.print("Wind = 2.6");
////    linkSerial.print("WindVane = 3");
//  }
//}
//DHT22


void readDHT(){
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  linkSerial.print("Temp = ");
  linkSerial.println(temperature);
  linkSerial.print("Hum = ");
  linkSerial.println(humidity);
  arrayTemperature[systemCounter] = temperature;
  arrayHumidity[systemCounter] = humidity;
  
}

//GPS


void readGPS(){
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  while(ss.available()){
    char c = ss.read();
    if(gps.encode(c)){
      newData = true;
    }
  }

  if(newData){
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    linkSerial.print("LAT=");
    lat = (flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    linkSerial.print(lat);
    linkSerial.print(" LON=");
    lon = (flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    linkSerial.print(lon);
    linkSerial.print(" SAT=");
    linkSerial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    linkSerial.print(" PREC=");
    linkSerial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
  }

  gps.stats(&chars, &sentences, &failed);
  linkSerial.print(" CHARS=");
  linkSerial.print(chars);
  linkSerial.print(" SENTENCES=");
  linkSerial.print(sentences);
  linkSerial.print(" CSUM ERR=");
  linkSerial.println(failed);
//  linkSerial.println("LAT=-7.762648");
//  linkSerial.println("LON=110.373583");
  if (chars == 0)
  linkSerial.println("--NO DATA, Check Wiring--");
  
}

//void testingAWS(){
//    for (int l = 0; l < 34; l++) { 
//      idAlat += char(EEPROM.read(l)); 
//    }
//    linkSerial.println(idAlat+"Send");
//    
//    String idString = String(idAlat);
//    idAlat = "";
//    
//    float tempString = 35;
//    float humString = 90;
//    float windString = 17.5;
//    float pressureString = 10010;
//    float latString = -7.762648;
//    float lonString = 110.373583;
//    float curahString = 55.3;
//    int windVaneString = 7;
////    StaticJsonDocument<200> doc;
////    doc["temperature"] = 35;
////    doc["humidity"] = 90;
////    doc["wind_speed"] = 17.5;
//    snprintf (msg, BUFFER_LEN, "{\"id_alat\" : \"%s\", \"temperature\" : %f, \"humidity\" : %f, \"wind_speed\" : %f, \"wind_direction\" : %d, \"latitude\" : %f, \"longitude\" : %f, \"rainfall\" : %f, \"pressure\" : %f}", idString.c_str(), tempString, humString, windString, windVaneString, latString, lonString, curahString, pressureString );
//    if(aws.publish(MQTT_TOPIC, msg) == 0){// publishes payload and returns 0 upon success
//      linkSerial.println("Success\n");
//    }
//    else{
//      linkSerial.println("Failed!\n");
//      testingAWS();
//    }
//}

//Another Variabel
int delaytime = 1000*10;
unsigned long previousMillis = 0;

//Main
void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(34);  
  pinMode(pin_interrupt, INPUT);

  pinMode(utara,INPUT_PULLUP);
  pinMode(tl,INPUT_PULLUP);
  pinMode(timur,INPUT_PULLUP);
  pinMode(tenggara,INPUT_PULLUP);
  pinMode(selatan,INPUT_PULLUP);
  pinMode(bd,INPUT_PULLUP);
  pinMode(barat,INPUT_PULLUP);
  pinMode(bl,INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(pin_interrupt), hitung_curah_hujan, FALLING);
  linkSerial.begin(9600);
  ss.begin(9600);
  wifiStart();
  dht.begin();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  
  if(flag == true){
    curah_hujan_per_menit += milimeter_per_tip;
    curah_hujan_per_jam += milimeter_per_tip;
    flag = false;
  }
  
  //receiveUART();
  
  if(currentMillis - previousMillis >= delaytime){
    previousMillis = currentMillis;
    //readBMP();
    readCurah();
    readDHT();
    readGPS();
    WindVane();
    receiveUARTNoJson();
    if(systemCounter == 59){
      sendAWS();
      systemCounter = 0;
      for(int i=0;i<60;i++){
        arrayTemperature[i] = 0;
        arrayHumidity[i] = 0;
        arrayPressure[i] = 0;
        arrayWindSpeed[i] = 0;
      }
    }
    systemCounter++;
    //testingAWS();
  }
    
  
}
