#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

#define LOM_PIN 4 // LO+ pin
#define LOP_PIN 5 // LO- pin
#define SAMPLING_TIME 60 // In seconds

// WiFi access point credentials

const char * ssid = "WiFi Name";
const char * password = "WiFi Password";

// MQTT server IP address and port

const char * mqttServer = "IP Address";
const int mqttPort = 1883;

// Global variables

bool stop = false; // Switch to prevent further sampling
String to_send; // Data to send
int time_start;
int count = 0;

// callback(): Print any messages received from the broker (unused function)

void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  Serial.println();

  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
    message += (char) payload[i];
  }

  Serial.println("-----------------------");
}

// Create WiFi and MQTT clients

WiFiClient espClient;
PubSubClient client(mqttServer, mqttPort, callback, espClient);

void setup() {
  Serial.begin(9600);

  pinMode(LOM_PIN, INPUT); // Setup for leads off detection LO-
  pinMode(LOP_PIN, INPUT); // Setup for leads off detection LO+

  // Connect to WiFi

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to RPi MQTT server...");

    if (client.connect("ESP8266Client")) {
      client.subscribe("ECG");
      Serial.println("Connected!");
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  time_start = millis();
}

void loop() {
  if (!stop) {
    if ((digitalRead(LOM_PIN) == 1) || (digitalRead(LOP_PIN) == 1)) { // If leads are not connected...
      if (count < 10) count++;
      else count = 0;
      to_send = String(count); // Data to send (for debugging purposes)
    } else {
      // Read ECG signal
      Serial.println(analogRead(A0));
      to_send = String(analogRead(A0));
    }

    client.publish("ECG/data", to_send.c_str()); // Publish to MQTT server on topic "ECG/data"

    // Wait for a bit to keep serial data from saturating

    delay(2);

    // Send timestamp

    int time_now = millis();
    to_send = String(time_now - time_start);
    client.publish("ECG/data", to_send.c_str());

    // Terminate after sampling time is over

    if (time_now - time_start > SAMPLING_TIME * 1000) {
      stop = true;
      Serial.println("Session terminated.");
    }
  }
}
