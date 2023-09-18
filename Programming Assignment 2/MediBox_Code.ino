#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

#define DHT_PIN 15
#define BUZZER 12
#define LDR_PIN 36              //Defining the pin for the LDR input. It is a analog output.Therefore I used the analog pin
#define SERVO_MOTOR_PIN 18      //Defining the pin for the servo motor

float minimum_angle=30.0;       // Defining the minimum angle as global variable. Using dashboard input we can change the minimum angle
float controlling_factor=0.75;  // Defining the controlling factor as global variable

Servo servo;

// Intializing the wifi client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Intializing the time client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Intializing the DHT sensor
DHTesp dhtSensor;

bool isScheduledON = false;
unsigned long scheduledOnTime;

char temAr[6];
char lightAr[6];

void setup() {
  Serial.begin(115200);  
  setupWifi();

  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(LDR_PIN, INPUT);  // Setup the LDR pin

  servo.attach(SERVO_MOTOR_PIN, 500, 2400); // Set up the servo motor
}

void loop() {
  if (!mqttClient.connected()){
    connectToBroker();
  }

  mqttClient.loop();

  updateTemperature();
  Serial.println(temAr);
  mqttClient.publish("CSE-ADMIN-TEMP",temAr);

  checkSchedule();

  readLightIntensity();       // Regular loop read the light intensity
  mqttClient.publish("CSE-ADMIN-LIGHT",lightAr);  // Publish the light intensity to the dashboard

  delay(1000);
}

void buzzerOn(bool on){
  if (on){
    tone(BUZZER, 256);
  }
  else{
    noTone(BUZZER);
  }
}

// Set up the mosquitto broker
void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883);
  mqttClient.setCallback(receiveCallback);
}

void receiveCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];

  for (int i=0;i<length;i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i]=(char)payload[i];
  }

  Serial.println(":");

  if (strcmp(topic,"CSE-ADMIN-MAIN-ON-OFF")==0){  
      buzzerOn(payloadCharAr[0]=='1');
  }
  else if (strcmp(topic,"CSE-ADMIN-SCH-ON")==0){
      if (payloadCharAr[0]=='N'){
        isScheduledON = false;
      }else{
        isScheduledON = true;
        scheduledOnTime = atol(payloadCharAr);
      }
  }
  // Receiving the minimum angle from the dashboarad
  if (strcmp(topic,"CSE-ADMIN-MINI-ANG")==0){
      minimum_angle = atof(payloadCharAr);
      Serial.println(minimum_angle);
  }
  // Receiving the controlling factor from the dashboarad
  if (strcmp(topic,"CSE-ADMIN-CON-FAC")==0){
      controlling_factor = atof(payloadCharAr);
      Serial.println(controlling_factor);
  }
}

void connectToBroker(){
  while (!mqttClient.connected()){
    Serial.println("Attempting MQTT connetion...");
    if (mqttClient.connect("ESP-1234567235")){
      Serial.println("Connected");
      mqttClient.subscribe("CSE-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("CSE-ADMIN-SCH-ON");
      mqttClient.subscribe("CSE-ADMIN-MINI-ANG");   // Subcribe for the new topics
      mqttClient.subscribe("CSE-ADMIN-CON-FAC");
    }
    else{
      Serial.println("failed");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void updateTemperature(){
  TempAndHumidity data=dhtSensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(temAr, 6);
}

void setupWifi(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST","");

  while (WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Wifi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule(){
  if (isScheduledON){
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime){
      Serial.println("Current :"+String(currentTime));
      Serial.println("Schedule :"+String(scheduledOnTime));
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("CSE-ADMIN-MAIN-ON-OFF-ESP","1");
      mqttClient.publish("CSE-ADMIN-SCH-ESP-ON","0");
      Serial.println("Scheduled ON");
    }
  }  
}

void readLightIntensity(){
  int analogValue = analogRead(LDR_PIN);
  // I used the linear interpolation to get the value between 0 and 1
  double floatAnalogValue=((32.00-analogValue)/4031.00)+1.00;
  String(floatAnalogValue,2).toCharArray(lightAr, 6);
  servoMotor(floatAnalogValue); 
  Serial.println(floatAnalogValue);
}

void servoMotor(double value){
  //Calculating the angle of the motor
  double angle = minimum_angle +(180.0-minimum_angle)*value*controlling_factor;
  servo.write(angle);  
}