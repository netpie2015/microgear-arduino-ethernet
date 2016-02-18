# microgear-arduino-ethernet

microgear-arduino-ethernet is a client library that is used to connect an Arduino board to the NETPIE Platform's service for developing IoT applications. For more details on the NETPIE Platform, please visit https://netpie.io . 

## Compatibility
This library can be used with Arduino Mega 2560 and  Ethernet Shield

Usage Example
```c++
#include <AuthClient.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <PubSubClient.h>
#include <SHA1.h>
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <MicroGear.h>

#define APPID       <APPID>
#define GEARKEY     <APPKEY>
#define GEARSECRET  <APPSECRET>
#define SCOPE       ""

EthernetClient client;
AuthClient *authclient;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
MicroGear microgear(client);
int timer = 0;

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Found new member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();  
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Lost member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setName("mygear");
}

void setup() {  
    Serial.begin(9600);
    Serial.println("Starting...");

    microgear.on(MESSAGE,onMsghandler);
    microgear.on(PRESENT,onFoundgear);
    microgear.on(ABSENT,onLostgear);
    microgear.on(CONNECTED,onConnected);

    if (Ethernet.begin(mac)) {
      Serial.println(Ethernet.localIP());
      microgear.resetToken();
      microgear.init(GEARKEY,GEARSECRET,SCOPE);
      microgear.connect(APPID);
    }
}

void loop() {
	if (microgear.connected()) {
		Serial.println("connected");
		microgear.loop();
		if (timer >= 1000) {
  			microgear.chat("mygear","Hello");
	    	timer = 0;
	    } 
    	else timer += 100;
	}
	else {
	    Serial.println("connection lost, reconnect...");
    	if (timer >= 5000) {
          microgear.connect(APPID);
			timer = 0;
		}
		else timer += 100;
	}
	delay(100);
}

```
## Library Usage
---
**microgear.init (*gearkey*, *gearsecret*, *scope*)**

**arguments**
* *gearkey* `string` - is used as a microgear identity.
* *gearsecret* `string` comes in a pair with gearkey. The secret is used for authentication and integrity. 
* *scope* `string` - specifies the right.  

**scope** is an optional field. This can be specified when the microgear needs additional rights beyond default scope. If the scope is specified, it may need an approval from the Application ID's owner for each request. The scope format is the concatenation of strings in the following forms, separated with commas:

  * [r][w]:&lt;/topic/path&gt; - r and w is the right to publish and subscribe topic as specified such as rw:/outdoor/temp
  *  name:&lt;gearname&gt; - is the right to name the &lt;gearname&gt;
  *  chat:&lt;gearname&gt; - is the right to chat with &lt;gearname&gt;
In the key generation process on the web netpie.io, the developer can specify basic rights to each key. If the creation of microgear is within right scope, a token will be automatically issued, and the microgear can be connected to NETPIE immediately. However, if the requested scope is beyond the specified right, the developer will recieve a notification to approve a microgear's connection. Note that if the microgear has operations beyond its right (e.g., pulishing to the topic that it does not has the right to do so), NETPIE will automatically disconnect the microgear. In case that APPKEY is used as a gearkey, the developer can ignore this attribute since by default the APPKEY will gain all rights as the ownwer of the app.

```c++
microGear.init("sXfqDcXHzbFXiLk",
               "DNonzg2ivwS8ceksykGntrfQjxbL98",
               "r:/outdoor/temp,w:/outdoor/valve,name:logger,chat:plant");
```