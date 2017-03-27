/*  A simple feed sample codes                                                    */
/*  You will need the following:                                                  */
/*    1. Subscribe to NETPIE.io.                                                  */
/*.   2. Create an APPID, KEY and SECRET and replace their values in the code.    */
/*    3. Create Feed and replace <FEEDID> and <FEEDAPIKEY> with the real values.  */ 
/*.   4. Create a feed attribute 'value' of type number.                          */

#include <Ethernet.h>
#include <MicroGear.h>

#define APPID   <APPID>
#define KEY     <KEY>
#define SECRET  <SECRET>
#define ALIAS   "feedsender"

#define FEEDID      <FEEDID>
#define FEEDAPIKEY  <FEEDAPIKEY>

EthernetClient client;
AuthClient *authclient;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };
MicroGear microgear(client);
int timer = 0;
String feedstr;

void onErrorHandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("\nError --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
}

void onInfoHandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("\nInfo --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setAlias(ALIAS);
}

void setup() {  
    Serial.begin(9600);
    Serial.println("Starting...");

    microgear.on(ERROR,onErrorHandler);   //Call this if you want to see error message
    microgear.on(INFO,onInfoHandler);     //Call this if you want to see info message
    microgear.on(CONNECTED,onConnected);

    if (Ethernet.begin(mac)) {
      Serial.println(Ethernet.localIP());
      microgear.init(KEY,SECRET,ALIAS);
      microgear.connect(APPID);
    }
}

void loop() {
  if (microgear.connected()) {
    microgear.loop();
    Serial.print(".");
    if (timer >= 15000) {
        // generate a demo value of sine wave
        feedstr = String("value:")+String(round(100.0*sin(2*3.1416*millis()/240000)));
        Serial.print("\nwrite to feed -- >"); Serial.println(feedstr);
        microgear.writeFeed(FEEDID,feedstr,FEEDAPIKEY);
        timer = 0;
      }
      else timer += 1000;
  }
  else {
      Serial.println("connection lost, reconnect...");
      if (timer >= 5000) {
          microgear.connect(APPID);
      timer = 0;
    }
    else timer += 1000;
  }
  delay(1000);
}
