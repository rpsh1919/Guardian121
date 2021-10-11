#include <Arduino.h>
#include "eQ3.h"
#include <BLEDevice.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define ADDRESS "00:00:00:00:00:00" // write your own
#define USER_KEY "00000000000000000000000000000000" // write your own
#define USER_ID 2

IPAddress ip(000, 000, 000, 000); // write your own
char ssid[] = "xxxxxxxx"; // write your own
char pass[] = "xxxxxxxx"; // write your own
int statusWiFi = WL_IDLE_STATUS;
WiFiClient  client;

eQ3* keyble;
bool do_open = false;
bool do_lock = false;
bool do_unlock = false;
bool do_status = true; 
unsigned long timeout=0;
bool statusUpdated=false;
bool waitForAnswer=false;
unsigned long starttime=0, LastUpShort;
int status = 0;


void Connect() {
  IPAddress gw(192, 168, 0, 1); // write your own
  IPAddress sn(255, 255, 255, 0); // write your own
  IPAddress dns(192, 168, 0, 1); // write your own
  WiFi.config(ip, gw, sn, dns);
  while ( statusWiFi != 6) {
    Serial.println("Connecting..."); 
    Serial.println(ssid);
    Serial.println(status);
    statusWiFi = WiFi.begin(ssid, pass);
    delay(5000);
  }
  Serial.println("Booting..."); 
  WiFi.mode(WIFI_STA);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Rebooting..."); 
    delay(5000);
    ESP.restart();
  }
}


void NewStatus() {
   yield();
   statusUpdated = false;
   status = keyble->_LockStatus;
   Serial.println(status);
   WiFi.begin(ssid, pass); 
}

void Lock() {
  WiFi.disconnect();  
  yield();
  waitForAnswer=true;
  keyble->_LockStatus = -1;
  starttime = millis();

      if (do_open) {
        Serial.println("unlock + open");
        //SetWifi(false);
        keyble->open();
        //SetWifi(true);
        do_open = false;
      }

      if (do_lock) {
        Serial.println("lock");
        //SetWifi(false);
        keyble->lock();
        //SetWifi(true);
        do_lock = false;
      }

      if (do_unlock) {
        Serial.println("unlock");
        //SetWifi(false);
        keyble->unlock();
        //SetWifi(true);
        do_unlock = false;
      }
       if (do_status) {
        Serial.println("Status");
        //SetWifi(false);
        keyble->updateInfo();
        //SetWifi(true);
        do_status = false;
      }
}

void WaitForAnswer(){
  bool timeout=(millis() - starttime > LOCK_TIMEOUT *1000 +1000);
  bool finished=false;

  if ((keyble->_LockStatus != -1) || timeout) {
        if(keyble->_LockStatus == 1) {
          if(timeout) {
              finished=true;
              Serial.println("Lockstatus 1 - timeout");
          }
        }
         else if(keyble->_LockStatus == -1) {
          if(timeout){
            keyble->_LockStatus = 9; //timeout
            finished=true;
            Serial.println("Lockstatus -1 - timeout");
          }
        }
        else if(keyble->_LockStatus != 1) {
           finished=true;
           Serial.println("Lockstatus != 1");
        }

        if(finished) {
          Serial.println("finshed.");
          do
          {
            keyble->bleClient->disconnect();
            delay(100);
          }
          while(keyble->state.connectionState != DISCONNECTED && !timeout);

          delay(100);
          yield();
          Serial.println("# Lock status changed or timeout ...");
          Serial.print("Data received: ");
          waitForAnswer=false;
          statusUpdated=true;
        }
      }
}

void setup() {
    Serial.begin(9600);
    Serial.println("Start..");
    BLEDevice::init("esp32ble");
    keyble = new eQ3(ADDRESS, USER_KEY, USER_ID);
    Connect();
     
}

void loop() {
  
   if ((millis() - LastUpShort) > 120000) {
     if (status == 2) { // unlocked
        do_lock = true;
     }
     else { // locked
        do_unlock = true;
     }
   }

   if (statusUpdated) NewStatus();
   if (do_open || do_lock || do_unlock || do_status) {
      Lock(); 
      LastUpShort = millis();
   }   

   if(waitForAnswer) WaitForAnswer();

}