#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Syslog.h>
#include "mqtt.h"
#include "main.h"
#include "app.h"


void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void publish_valves ();

WiFiClient espClient;
PubSubClient mqtt_client(espClient);


void mqtt_setup() {
    mqtt_client.setServer(MQTT_BROKER, 1883);
    mqtt_client.setCallback(callback);

    strcpy(mqtt_maintopic, DEFAULT_MAINTOPIC);
}


void mqtt_loop() {

    static unsigned long timer = millis();
    
    if (!mqtt_client.connected()) {
        reconnect();        
    }
    mqtt_client.loop();

    if (millis() > (uint32_t) 2000 + timer) {
        timer = millis();

        publish_valves ();

        //Serial.println("mqtt publish valves");
    }    

}


void reconnect() {
    char topicstr[sizeof(mqtt_maintopic)+20];
    char nrstr[3];

    while (!mqtt_client.connected()) {
        Serial.println("Reconnecting MQTT...");
        if (!mqtt_client.connect("ESP8266Client")) {
            Serial.print("failed, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }

    // make some subscriptions
    for (unsigned int x = 1;x<=ACTUATOR_COUNT;x++) {
        topicstr[0] = '\0';
        nrstr[0] = '\0';
        itoa(x, nrstr, 10);

        // prepare prefix
        strcat(topicstr, mqtt_maintopic);
        strcat(topicstr, nrstr);      

        // target value
        strcat(topicstr, "/target");
        mqtt_client.subscribe(topicstr);
        Serial.print("subscribing to ");
        Serial.println(topicstr);        
    }

    //mqtt_client.subscribe("/VdMotFBH/valve/1/target");
    //mqtt_client.subscribe("/VdMotFBH/valve/11/target");
    Serial.println("MQTT Connected...");
}


void callback(char* topic, byte* payload, unsigned int length) {
    unsigned char found = 0;
    unsigned char value;

    //Serial.print("Received message [");
    //Serial.print(topic);
    //Serial.print("] ");
    char msg[length+1];
    for (unsigned int i = 0; i < length; i++) {
        //Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    //Serial.println();
 
    msg[length] = '\0';
    //Serial.println(msg);

    //Serial.print("t"); Serial.println(&topic[0]);
    //Serial.print("i"); Serial.println(&mqtt_maintopic[0]);
 
    // test topic
    for(unsigned int x = 0; x < strlen(topic); x++) {
        if(topic[x] == '\0') break;

        if(topic[x] == mqtt_maintopic[x]) {
            found++;        
        }
        else if (mqtt_maintopic[x] == '\0' && strlen(mqtt_maintopic) == found) {
            break;
        }
        else {
            found = 0;
            break;
        }
    }

    //Serial.print("found: ");    Serial.println(found, 10);

    // find out valve nr
    if(found) {
        // single digit
        if (topic[found+1] == '/') {
            //Serial.println("single digit");
            //value = atoi(topic[found]);
            value = topic[found] - 48;
            found += 2;
            //Serial.println(value, 10);
        }
        else if (topic[found+2] == '/') {
            //Serial.println("two digits");
            value = (topic[found] - 48) * 10 + topic[found] - 48;
            found += 3;
            //Serial.println(value, 10);
        }
        else found = 0;
    }

    // find out subtopic
    if (found) {
        // subtopic "target"
        //Serial.println(&topic[found]);
        if (0 == strcmp(&topic[found],"target")) {
            //Serial.println("found target");
            syslog.log(LOG_INFO, "MQTT: found target topic");
            actuators[value-1].target_position = atoi(msg);
        }
        else {            
            found = 0;
        }
    }
    else syslog.log(LOG_ERR, "MQTT: cant eval data");


    // if(strcmp(msg,"on")==0){
    //     //digitalWrite(13, HIGH);
    //     Serial.println("on");
    // }
    // else if(strcmp(msg,"off")==0){
    //     //digitalWrite(13, LOW);
    //     Serial.println("on");
    // }
}


void publish_valves () {

    char topicstr[sizeof(mqtt_maintopic)+20];
    char nrstr[3];
    char valstr[10];
    unsigned char len;

    for (unsigned int x = 1;x<=ACTUATOR_COUNT;x++) {
        topicstr[0] = '\0';
        nrstr[0] = '\0';
        itoa(x, nrstr, 10);

        // prepare prefix
        strcat(topicstr, mqtt_maintopic);
        strcat(topicstr, nrstr);
        len = (unsigned char) strlen(topicstr);

        // actual value
        strcat(topicstr, "/actual");
        itoa(actuators[x-1].actual_position, valstr, 10);        
        mqtt_client.publish(topicstr, valstr);
        //Serial.println(&topicstr[0]);
        //delay(10);

        // state
        topicstr[len] = '\0';
        strcat(topicstr, "/state");
        itoa(actuators[x-1].state, valstr, 10);
        mqtt_client.publish(topicstr, valstr);
        //Serial.println(&topicstr[0]);
        //delay(10);

        // meancurrent
        topicstr[len] = '\0';
        strcat(topicstr, "/meancur");
        itoa(actuators[x-1].meancurrent, valstr, 10);
        mqtt_client.publish(topicstr, valstr);
        //Serial.println(&topicstr[0]);
        //delay(10);

        // temperature
        topicstr[len] = '\0';
        strcat(topicstr, "/temperature");
        itoa(actuators[x-1].temperature, valstr, 10);
        mqtt_client.publish(topicstr, valstr);
        //Serial.println(&topicstr[0]);
        //delay(10);
    }
}