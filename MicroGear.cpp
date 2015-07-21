#include "MicroGear.h"

void (* callback)(char* topic, uint8_t*, unsigned int);

void msgCallback(char* topic, uint8_t* payload, unsigned int length) {
	#ifdef DEBUG_H
		Serial.println("Incoming message-->");
	#endif
	callback(topic,payload,length);
}

bool MicroGear::clientReadln(Client* client, char *buffer, size_t buflen) {
		size_t pos = 0;
		while (true) {
			while (true) {
				uint8_t byte = client->read();
				if (byte == '\n') {
					// EOL found.
					if (pos < buflen) {
						if (pos > 0 && buffer[pos - 1] == '\r')
						pos--;
						buffer[pos] = '\0';
					}
					else {
						buffer[buflen - 1] = '\0';
					}
					return true;
				}
				if (pos < buflen)
				buffer[pos++] = byte;
			}
		}
		return false;
}

bool MicroGear::getHTTPReply(Client *client, char *buff, size_t buffsize) {
	char pline = 0;
	while (true) {
		clientReadln(client, buff, buffsize);
		if (pline > 0) {
			return true;
		}
		if (strlen(buff)<6) pline++;
	}
}

void MicroGear::syncTime(Client *client, unsigned long *bts) {
    char timestr[200];
    strcpy_P(timestr,PSTR("GET /api/time HTTP/1.1\r\n\r\n"));

    client->connect(GEARTIMEADDRESS,GEARTIMEPORT);
    client->write((const uint8_t *)timestr,strlen(timestr));

    delay(1000);
    getHTTPReply(client,timestr,250);
    
    *bts = atol(timestr) - millis()/1000;

    client->stop();
}

MicroGear::MicroGear(Client& netclient, void (* _callback)(char* topic, uint8_t*,unsigned int) ) {
    callback = _callback;

    sockclient = &netclient;
    constate = CLIENT_NOTCONNECT;

    this->token = NULL;
    this->tokensecret = NULL;
    this->backoff = 10;
    this->retry = RETRY;
}

void MicroGear::readEEPROM(char* buff,int offset,int len) {
    int i;
    for (i=0;i<len;i++) {
        buff[i] = EEPROM.read(offset+i);
    }
    buff[len] = '\0';
}

void MicroGear::writeEEPROM(char* buff,int offset,int len) {
    int i;
    for (i=0;i<len;i++) {
        EEPROM.write(offset+i,buff[i]);
    }
}

void MicroGear::resetToken() {
    char state[2];
    *state = EEPROM_STATE_NUL;
    writeEEPROM(state,EEPROM_STATEOFFSET,1);
}


void MicroGear::getToken(char* token, char* tokensecret, char *endpoint) {
    char state[2];
    int authstatus = 0;

    char rtoken[TOKENSIZE+1];
    char rtokensecret[TOKENSECRETSIZE+1];

    token[0] = '\0';
    tokensecret[0] = '\0';

    readEEPROM(state,EEPROM_STATEOFFSET,1);

	#ifdef DEBUG_H
		Serial.print("Token rom state == ");
		Serial.println(state);
	#endif
	delay(500);
    // if token is null  --> get a requesttoken
    if (*state != EEPROM_STATE_REQ && *state != EEPROM_STATE_ACC) {

        do {
			delay(2000);
            if (authclient->connect()) {

                authstatus = authclient->getGearToken(_REQUESTTOKEN,rtoken,rtokensecret,endpoint,gearkey,gearsecret,scope,NULL,NULL);
				delay(1000);
				#ifdef DEBUG_H
					Serial.println(authstatus); Serial.println(rtoken); Serial.println(rtokensecret); Serial.println(endpoint);
				#endif
                authclient->stop();

                if (authstatus == 200) {
                    *state = EEPROM_STATE_REQ;
                    writeEEPROM(state,EEPROM_STATEOFFSET,1);
                    writeEEPROM(rtoken,EEPROM_TOKENOFFSET,TOKENSIZE);
                    writeEEPROM(rtokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);

					#ifdef DEBUG_H
						Serial.println("@ Write Request Token");
		                Serial.println(state);
			            Serial.println(rtoken);
				        Serial.println(rtokensecret);
					#endif
                }
            }
            else delay(2000);
        } while (!authstatus);
    }

    //if token is a requesttoken --> get an accesstoken
    if (*state == EEPROM_STATE_REQ) {

        readEEPROM(rtoken,EEPROM_TOKENOFFSET,TOKENSIZE);
        readEEPROM(rtokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
		
        do {
            delay(1000);
            if (authclient->connect()) {
                authstatus = authclient->getGearToken(_ACCESSTOKEN,token,tokensecret,endpoint,gearkey,gearsecret,NULL,rtoken,rtokensecret);
				delay(1000);
                authclient->stop();
            }
            else {
                authstatus  = 0;
            }
            Serial.println(authstatus);
            if (authstatus == 200) {

                *state = EEPROM_STATE_ACC;
                writeEEPROM(state,EEPROM_STATEOFFSET,1);
                writeEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
                writeEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);

				char *p = endpoint+12;
				while (*p != '\0') {
					if (*p == '%') {
						*p = *(p+1)>='A'?16*(*(p+1)-'A'+10):16*(*(p+1)-'0');
						*p += *(p+2)>='A'?(*(p+2)-'A'+10):(*(p+2)-'0');
						strcpy(p+1,p+3);
					}
					p++;
				}
				writeEEPROM(endpoint+12,EEPROM_ENDPOINTSOFFSET,MAXENDPOINTLENGTH);
            }
            else {
                if (authstatus != 401) {
					#ifdef DEBUG_H
						Serial.println("Return code is not 401");
						Serial.println(authstatus);
					#endif

                    /* request token error e.g. revoked */
                    *state = EEPROM_STATE_NUL;
                    writeEEPROM(state,EEPROM_STATEOFFSET,1);
                    break;
                }
            }
        }while (*state == EEPROM_STATE_REQ);
	        // reset accesstoken retry counter
        retry = RETRY;
		#ifdef DEBUG_H
	        Serial.println(authstatus); Serial.println(token); Serial.println(tokensecret); Serial.println(endpoint);
		#endif
    }

    //if token is a requesttoken --> get an accesstoken
    if (*state == EEPROM_STATE_ACC) {
        readEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
        readEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
        readEEPROM(endpoint,EEPROM_ENDPOINTSOFFSET,MAXENDPOINTLENGTH);
    }

    if (*state != EEPROM_STATE_ACC) {
        #ifdef DEBUG_H
            Serial.println("Fail to get a token.");
        #endif
        delay(2000);
    }
	authclient->stop();
}

boolean MicroGear::connect(char* appid) {
    char username[USERNAMESIZE+1];
    char password[PASSWORDSIZE+1];
    char buff[2*TOKENSECRETSIZE+2];
    char token[TOKENSIZE+1];
    char tokensecret[TOKENSECRETSIZE+1];
    char endpoint[MAXENDPOINTLENGTH+1];

	syncTime(sockclient, &bootts);

    this->appid = appid;
	authclient = new AuthClient(*sockclient);
    authclient->init(appid,scope,bootts);
    getToken(token,tokensecret,endpoint);
	delete(authclient);

    /* generate one-time user/password */
    // To be changed --***************-->>>>
    sprintf(username,"%s%%%s%%%lu",token,"AR01",bootts+millis()/1000);
    sprintf(buff,"%s&%s",tokensecret,gearsecret);
    Sha1.initHmac((uint8_t*)buff,strlen(buff));
    Sha1.HmacBase64(password, username);

    if (*token && *tokensecret) {

		#ifdef DEBUG_H
			Serial.println("Going to connect to MQTT broker");
			Serial.println(token);
			Serial.println(username);
			Serial.println(password);
			Serial.println(endpoint);
		#endif

		char *p = endpoint;
		while (*p!='\0') {
			if (*p == ':') {
				*p = '\0';
				p++;
				break;
			}
			p++;
		}

		mqttclient = new PubSubClient(endpoint, *p=='\0'?1883:atoi(p), callback, *sockclient);

		delay(500);
		
	    constate = this->mqttclient->connect(token,username+TOKENSIZE+1,password);
		switch (constate) {
			case CLIENT_CONNECTED :
				    backoff = MINBACKOFFTIME;
					break;
            case CLIENT_NOTCONNECT :
	                if (backoff < MAXBACKOFFTIME) backoff = 2*backoff;
		            delay(backoff);
			        break;
            case CLIENT_REJECTED :
					#ifdef DEBUG_H
						Serial.println(retry);
					#endif
		            if (--retry <= 0) {
			            resetToken();
						#ifdef DEBUG_H
					        Serial.println("Reset token");
						#endif
					}
                    break;
	    }
		return constate;
    }
    else return false;
}

boolean MicroGear::connected() {
	if (constate == CLIENT_NOTCONNECT) return CLIENT_NOTCONNECT;
	else return this->mqttclient->connected();
	//return this->sockclient->connected();
}

void MicroGear::subscribe(char* topic) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    mqttclient->subscribe(top);
}

void MicroGear::unsubscribe(char* topic) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    mqttclient->unsubscribe(top);
}

void MicroGear::publish(char* topic, char* message) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    mqttclient->publish(top, message);
}

void MicroGear::setName(char* gearname) {
    char top[MAXTOPICSIZE];
    if (this->gearname) {
        strcpy(top,"/gearname/");
        strcat(top,this->gearname);
        unsubscribe(top);
    }
    strcpy(top,"/gearname/");
    strcat(top,gearname);
	this->gearname = gearname;
    subscribe(top);
}

void MicroGear::chat(char* targetgear, char* message) {
    char top[MAXTOPICSIZE];
	sprintf(top,"/%s/gearname/%s",appid,targetgear);
    mqttclient->publish(top, message);
}

int MicroGear::init(char* gearkey,char* gearsecret,char* scope) {
    //this->gearid = gearkey;
    this->gearkey = gearkey;
    this->gearsecret = gearsecret;
    this->scope = scope;
}

void MicroGear::loop() {
	this->mqttclient->loop();
}

void MicroGear::setToken(char* token,char* tokensecret) {
    char state[2];
    this->token = token;
    this->tokensecret = tokensecret;
    *state = EEPROM_STATE_ACC;
    writeEEPROM(state,EEPROM_STATEOFFSET,1);
    writeEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
    writeEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
}

