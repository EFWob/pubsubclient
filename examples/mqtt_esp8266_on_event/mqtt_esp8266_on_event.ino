/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "myTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = ".........";
const char* password = "............";
const char* mqtt_server = "broker.mqtt-dashboard.com";

#define myTopic "mqttOn"


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callbackLED1(char* topic, byte* payload, unsigned int length) {
  Serial.print("LED1-Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void callbackLED2(char* topic, byte* payload, unsigned int length) {
  Serial.print("LED2-Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (length == 0) 
    if (NULL != strstr(topic, myTopic "/led/switch/1"))   
    // Switch on the LED if topic is myTopic/led/1 with empty payload
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
    else if (NULL != strstr(topic, myTopic "/led/switch/0"))
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
}

void callbackLEDother(char* topic, byte* payload, unsigned int length) {
  Serial.println("Invalid LED message on MQTT. Please use:");
  Serial.println(myTopic "/led/1 with empty payload to switch LED on; or");
  Serial.println(myTopic "/led/0 with empty payload to switch LED off; or");
  Serial.println(myTopic "/led/switch/1 with empty payload to switch LED on; or");
  Serial.println(myTopic "/led/switch/0 with empty payload to switch LED off; or");
  Serial.println(myTopic "/led with payload '1' to switch LED on; or");
  Serial.println(myTopic "/led with payload '0' to switch LED off.");
  Serial.println();
  Serial.println("Thank you for your cooperation");
}


void callbackRestart(char* topic, byte* payload, unsigned int length) {
  Serial.println("Restart-Message arrived. Ignoring Payload. Disconnecting client. See if autosubscribe works after...");
  Serial.println();
  client.disconnect();delay(500);
}




void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(myTopic, "hello world");
      // ... and resubscribe
//      client.subscribe(myTopic "/in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  // Each call to "on()" will link the given topic to a defined callback
  // Topics can be defined using the wildCard-Scheme of MQTT (i. e. '+' and '#')
  // Syntax on wildcards is not checked. Illegal expressions might lead to undefined behaviour
  // (like topic= "foo++")
  // (there is however an auto-cut after the first '#' encountered)
  //
  // If topics overlap, they are resolved in the same order as called.
  // Logic is: first "on()" definition that matches, does fire. 
  // If no "on()" definition fires, the overall callback is called (if given).
  // i. e. "on()" definitions always take precedence over "subscribe()"
  // on ESP8266/ESP32 lambdas can be used (see below)
  // "on()" descriptions can be defined anytime a client is available. Client does not need to be connected.
  // Resulting subscriptions will be resolved in "::loop()", if client is connected.
  // Each call to "::loop()" will add one subscription if connected. (To avoid flooding)
  // Therefore it is possible, that a overlapping subscription from "::subscribe()" will fire the central
  // callback (if defined), if the subscription resulting from respective "::on()"  
  // 
  // All incoming messages are compared to all defined "::on()"-Topics till match.
  // If possible, the high frequency messages should be catched first.
  //
  // Now for the examples...
  // 'myTopic' stands for #define myTopic above
  //
  // if "'myTopic'/led" is received, use callbackLED1 (switches LED on/off if payload is precise '1'/'0'
  
  client.on(myTopic "/led", callbackLED1); 

  // if "'myTopic'/led/switch/+" is received, use callbackLED2
  // if payload is empty, and topic is precise .../switch/1 or .../switch/0, LED will be switched accordingly

  client.on(myTopic "/led/switch/+", callbackLED2);

  // demo of using lambdas to directly switching LED on/off with "'myTopic'/led/1" or "'myTopic'/led/0"
  
  client.on(myTopic "/led/1", [](char *topic, uint8_t* payload, int payloadLen) {digitalWrite(BUILTIN_LED, LOW);});
  client.on(myTopic "/led/0", [](char *topic, uint8_t* payload, int payloadLen) {digitalWrite(BUILTIN_LED, HIGH);});

  // note that on each match above the message is considered consumed. Even if the payload is not as expected.
  // that means, the following will never fire on any of the 'led' messages defined above. Only on any other message
  // starting with "'myTopic'/led"
  
  client.on(myTopic "/led/#", callbackLEDother);
 
  // the following topic will disconnect the client. The main loop() below includes the reconnect attempt.
  // after a reconnect, the client will again re-subscribe automatically to all topics defined by "::on()" methods (in order).
  
  client.on(myTopic "/restart", callbackRestart);

  // garbage collection for "anything else"
  
  client.on(myTopic "/#", callback);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
