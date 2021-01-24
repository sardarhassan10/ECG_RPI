//libraries
#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>    //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
//change these according to your pin numbers 
#define LO_plus 10
#define LO_minus 11
#define input A0
#define time_period 5

int time_start;
const char* mqttServer = "192.168.0.110";  //change server
const int mqttPort = 1883;      // change port

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  Serial.println();
  
  String message = "";
  for (int i = 0; i < length; i++) {
   Serial.print((char)payload[i]);
    message+=(char)payload[i];
  }
  Serial.println("-----------------------");
//  if(String(topic)=="LED"){
//    if(message=="LED ON"){
//      //digitalWrite(LED,HIGH);
//      Serial.println("Message received");
//    }
//    else{
//      //digitalWrite(LED,LOW);
//      Serial.println("Message not received");
//    }
//  }
     
}

WiFiClient espClient;
PubSubClient client(mqttServer, mqttPort, callback, espClient);

void setup() {
  // initialize the serial communication:
  Serial.begin(9600);
     
  WiFiManager wifiManager;
  //first parameter is name of access point, second is the password
  wifiManager.autoConnect("AP-NAME", "AP-PASSWORD");
  Serial.println("connected");

   client.setServer(mqttServer, mqttPort);
   client.setCallback(callback);
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client" )) {
      client.subscribe("LED");
      Serial.println("connected");
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
}
  // change pin numbers
  pinMode(LO_plus, INPUT); // Setup for leads off detection LO +
  pinMode(LO_minus, INPUT); // Setup for leads off detection LO -
  time_start=millis();
}

void loop() {
  // put your main code here, to run repeatedly:

  //Wait for a bit to keep serial data from saturating
  delay(2);
  int time_now= millis();
  int time_to_send = time_now-time_start;
  if (time_to_send >= time_period){
    
    if((digitalRead(LO_plus) == 1)||(digitalRead(LO_minus) == 1)){
    Serial.println('!');
    String toSend = String('!');
    client.publish("data",toSend.c_str());
  }
  else{
    // send the value of analog input 0:
      Serial.println(analogRead(input)); /// change value of port
      String toSend = String(analogRead(input));
      client.publish("data",toSend.c_str());
  }  
  Serial.println(time_to_send);
  String toSend_1 = String(time_to_send);
  client.publish("data",toSend_1.c_str());
  time_start=millis();
  }


}
