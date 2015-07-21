#include <AuthClient.h>
#include <debug.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <PubSubClient.h>
#include <SHA1.h>

#include <Arduino.h>

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <MicroGear.h>

#define APPID       "piedemo"
#define GEARKEY     "qDDwMaHEXfBiXmL"
#define GEARSECRET  "vNoswuhfqjxWSm0GR7cycGPniekw03"
#define SCOPE       ""                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                

EthernetClient client;
AuthClient *authclient;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

int p7counter = 0;

void msghandler(char *topic, uint8_t* msg, unsigned int msglen) {
	Serial.print("Incoming message-->");
	Serial.print(topic);
	p7counter = 2;
	digitalWrite(7,HIGH);
}

MicroGear microgear(client, msghandler);

int laststate = -1;

void setup() {  
    pinMode(5, INPUT);
    pinMode(7, OUTPUT);

    Serial.begin(9600);
    Serial.println("Starting...");

    if (Ethernet.begin(mac)) {
      Serial.println("dddd");
      Serial.println(Ethernet.localIP());
      microgear.resetToken();
      microgear.init(GEARKEY,GEARSECRET,SCOPE);
      microgear.connect(APPID);
      microgear.setName("nobita");
     microgear.subscribe("/test");
    }
}

void loop() {
  Serial.println("loop");
	if (microgear.connected()) {
    Serial.println("connected");
		microgear.loop();
		if (digitalRead(5) == HIGH) {
			if (laststate != HIGH) {
				laststate = HIGH;
				microgear.publish("/test","Pressed");
			}
		}
		else laststate = LOW;
	}
	else {
        	Serial.println("xxxxxx");
	}
	if (p7counter > 0) {
	    p7counter--;
	    if (p7counter == 0) digitalWrite(7,LOW);
	}
	delay(100);
}
