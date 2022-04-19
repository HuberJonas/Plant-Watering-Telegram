#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <UniversalTelegramBot.h>

#include "user/user.h"
#include "settings.h"

void checkMoisture();
void waterPlant();
void updateBot();
void handleMessage(String chatID, String text);

WiFiClient client;
UniversalTelegramBot bot(BOT_TOKEN, client);
unsigned long lastMillisUpdateBot;
unsigned long lastMillisCheckMoisture;



/**
 * 
 * Setup and Loop
 * 
 */
void setup() {
  Serial.begin(9600);
  Serial.println();

  // connect wifi
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Success");

  pinMode(PUMP_PIN, OUTPUT);
}

void loop() {
  if(millis() - lastMillisUpdateBot >= UPDATE_INTERVAL) {
    updateBot();
    lastMillisUpdateBot = millis();
  }

  if(millis() - lastMillisCheckMoisture >= UPDATE_INTERVAL) {
    checkMoisture();
    lastMillisCheckMoisture = millis();
  }
}



/**
 * 
 * Plant management
 * 
 */
void checkMoisture() {
  // read value
  int reading = 0;
  for (int i = 0; i < 5; i++) {
    reading += analogRead(SENSOR_PIN);
    delay(50);
  }
  reading = (int)reading/5;
  
  // water if necessary
  if(reading < THRESHOLD) {
    waterPlant();
  }
}


void waterPlant() {
  Serial.println();
  Serial.println("Watering the plant");
  digitalWrite(PUMP_PIN, HIGH);
  delay(PUMP_INTERVAL);
  digitalWrite(PUMP_PIN, LOW);
  // TODO volume control
}



/**
 * 
 * Bot management
 * 
 */
String waitingVerification = "";
String waitingName = "";
String waitingDeletion = "";


void updateBot() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if(numNewMessages > 0) {
    handleMessage(bot.messages[0].chat_id, bot.messages[0].text);
  }
}


void handleMessage(String chatID, String text) {
  //TODO real
  User user();
  bool verified = true;

  if(!verified) {
    if(waitingVerification == chatID) {
      if(text == SECRET) {
        bot.sendMessage(chatID, "Plant you can call me. Wie darf ich dich nennen?");
        waitingVerification = "";
        waitingName = chatID;
      }
      else {
        bot.sendMessage(chatID, "Leider Falsch. Du kannst auch eine registrierte Person bitten /secret einzugeben.");
      }
    }
    else {
      bot.sendMessage(chatID, "Willkommen, bitte gib den Schlüssel ein.");
      waitingVerification = chatID;
    }
  }
  else{
    if(waitingName == chatID) {
      //TODO save user
      bot.sendMessage(chatID, "Willkommen NAME");
      waitingName = "";
    }
    else if(waitingDeletion == chatID) {
      if(text == "Delete") {
        //TODO delete user
        bot.sendMessage(chatID, "Schade. Vielleicht sehen wir uns nochmal.\nTschüss"),
        waitingDeletion = "";
      }
      else{
        bot.sendMessage(chatID, "Vorgang abgebrochen.");
        waitingDeletion = "";
      }
    }

    else if(text == "/stats") {
      bot.sendMessage(chatID, "Stats einfügen");
    }
    else if(text == "/secret") {
      bot.sendMessage(chatID, "Hier kommt der Schlüssel. Teile ihn nicht mit jedem!");
      bot.sendMessage(chatID, SECRET);
    }
    else if(text == "/deleteMe") {
      bot.sendMessage(chatID, "ACHTUNG\nBestätige das Löschen, indem du \"Delete\" schickst.");
      waitingDeletion = chatID;
    }
    else if(text == "/water") {
      bot.sendMessage(chatID, "Wie du befiehlst. Ich gieße!");
      waterPlant();
    }
  }

}
