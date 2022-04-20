#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <UniversalTelegramBot.h>

#include "settings.h"
#include "eeprom_handler.h"

void checkMoisture();
void waterPlant();
void updateBot();
void handleMessage(String chatID, String text, String name);
void notifyUsers();
boolean verifyUser(String chatID);
void addUser(String chatID);
void deleteUser(String chatID);

WiFiClient client;
UniversalTelegramBot bot(BOT_TOKEN, client);
boolean usersHaveBeenNotified = false;
unsigned long lastMillisUpdateBot;
unsigned long lastMillisCheckMoisture;

int threshold = 0;
int volume = 0;
int registeredUsers = 0;
String users[AMOUNT_USERS];


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

  // Pump and EEPROM
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
  EEPROM.begin(AMOUNT_USERS*20+11);

  threshold = readThershold();
  volume = readVolume();

  registeredUsers = readRegisteredUsers();
  for(int i=0; i<registeredUsers; i++) {
    users[i] = readChatID(i);
  }
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
  if(reading < threshold) {
    waterPlant();

    if(volume <= 50 && usersHaveBeenNotified == false) {
      notifyUsers();
      usersHaveBeenNotified = true;
    }
    else if (volume > 50) {
      usersHaveBeenNotified = false;
    }
  }
}


void waterPlant() {
  Serial.println();
  Serial.println("Watering the plant");
  digitalWrite(PUMP_PIN, LOW);
  delay(PUMP_INTERVAL);
  digitalWrite(PUMP_PIN, HIGH);
  volume -= PUMP_VOLUME;
  writeVolume(volume);
}



/**
 * 
 * Bot management
 * 
 */
String waitingVerification = "";
String waitingDeletion = "";
String waitingThreshold = "";

void updateBot() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if(numNewMessages > 0) {
    handleMessage(bot.messages[0].chat_id, bot.messages[0].text, bot.messages[0].from_name);
  }
}


void handleMessage(String chatID, String text, String name) {
  bool verified = verifyUser(chatID);

  if(!verified) {
    if(waitingVerification == chatID) {
      if(text == SECRET) {
        bot.sendMessage(chatID, "Plant you can call me. Willkommen " + name + ".");
        waitingVerification = "";
        addUser(chatID);
      }
      else {
        bot.sendMessage(chatID, "Leider Falsch. Du kannst auch eine registrierte Person bitten /secret einzugeben.");
      }
    }
    else {
      if(registeredUsers >= AMOUNT_USERS) {
        bot.sendMessage(chatID, "Um diese Pflanze kümmern sich schon zu viele Leute. Es ist leider kein Platz mehr frei.");
      }
      else {
        bot.sendMessage(chatID, "Willkommen, bitte gib den Schlüssel ein.");
        waitingVerification = chatID;
      }
    }
  }
  else{
    if(waitingDeletion == chatID) {
      if(text == "Delete") {
        bot.sendMessage(chatID, "Schade. Vielleicht sehen wir uns nochmal.\nTschüss " + name);
        waitingDeletion = "";
        deleteUser(chatID);
      }
      else{
        bot.sendMessage(chatID, "Vorgang abgebrochen.");
        waitingDeletion = "";
      }
    }
    else if(waitingThreshold == chatID) {
      int threshold = 0;
      try {
        int threshold = (int)text;
        writeThershold(threshold);
        bot.sendMessage(chatID, "Neuer Schwellwert gesetzt.");
      }
      catch(const std::exception& e) {
        bot.sendMessage(chatID, "Bitte schicke eine Zahl.");
      }
    }

    else if(text == "/stats") {
      int fillPercentage = (int)(volume/WATER_VOLUME)*100;
      bot.sendMessage(chatID, "Die Tankfüllung liegt bei " + fillPercentage + "%.");
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
    else if(text == "/sensorRaw") {
      bot.sendMessage(chatID, "Der Messwert des Sensors liegt bei " + analogRead(SENSOR_PIN) + ".");
    }
    else if(text == "/setThreshold") {
      bot.sendMessage(chatID, "Bitte gib den neuen Schwellwert für die Bewässerung ein.");
      waitingThreshold = chatID;
    }
  }
}

void notifyUsers() {
  for(int i=0; i<registeredUsers; i++) {
    bot.sendMessage(users[i], "Stille Wasser sind zwar tief, aber der Wasserstand im Tank wird langsam zu gering.");
  }
}



/**
 * 
 * User management
 * 
 */
boolean verifyUser(String chatID) {
  for(int i=0; i<registeredUsers; i++) {
    if(users[i] == chatID) {
      return true;
    }
  }
  return false;
}

void addUser(String chatID) {
  if(registeredUsers >= AMOUNT_USERS) return;
  writeChatID(registeredUsers, chatID);
  users[registeredUsers] = chatID;
}

void deleteUser(String chatID) {
  if(registeredUsers == 0) return;
  String lastUser = users[registeredUsers-1];

  // find user to delete
  for(int i=0; i<registeredUsers; i++) {
    if(users[i] == chatID) {
      users[i] = lastUser;
      users[registeredUsers-1] = "";
    }
  }
}
