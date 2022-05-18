#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "settings.h"
#include "eeprom_handler.h"

void checkMoisture();
void waterPlant();
int readMoisture(int amountReadings);
void updateBot();
void handleMessage(String chatID, String text, String name);
void notifyUsers();
boolean verifyUser(String chatID);
void addUser(String chatID);
void deleteUser(String chatID);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure clientSecure;
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);
boolean usersHaveBeenNotified = false;
unsigned long lastMillisUpdateBot;
unsigned long lastMillisCheckMoisture;

int threshold = 0;
int tankFill = 0;
int amountRegisteredUsers = 0;
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
  Serial.print("Connecting to Wifi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  clientSecure.setTrustAnchors(&cert);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Success");

  // Pump and EEPROM
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
  EEPROM.begin(AMOUNT_USERS*15+10);

  threshold = readMoistureThreshold();
  tankFill = readVolume();
  tankFill = 100;

  amountRegisteredUsers = readAmountRegisteredUsers();
  for(int i=0; i<amountRegisteredUsers; i++) {
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
  int reading = readMoisture(3);
  
  // water if necessary
  if(reading < threshold) {
    waterPlant();
  }

  // check if water level is critical
  if(tankFill <= PUMP_VOLUME*2 && usersHaveBeenNotified == false) {
    Serial.println("Critical tank fill reached.");
    notifyUsers();
    usersHaveBeenNotified = true;
  }
  else if (tankFill > PUMP_VOLUME*2) {
    usersHaveBeenNotified = false;
  }
}


void waterPlant() {
  Serial.println("Watering the plant");
  digitalWrite(PUMP_PIN, LOW);
  delay(PUMP_INTERVAL);
  digitalWrite(PUMP_PIN, HIGH);
  tankFill -= PUMP_VOLUME;
  writeVolume(tankFill);
}


int readMoisture(int amountReadings) {
  // read value
  int reading = 0;
  for (int i = 0; i < amountReadings; i++) {
    reading += analogRead(SENSOR_PIN);
    delay(50);
  }
  return (int)reading/5;
}



/**
 * 
 * Bot management
 * 
 */
String waitingVerification = "";
String waitingDeletion = "";
String waitingThreshold = "";
String waitingReset = "";

void updateBot() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if(numNewMessages > 0) {
    handleMessage(bot.messages[0].chat_id, bot.messages[0].text, bot.messages[0].from_name);
  }
}


void handleMessage(String chatID, String text, String name) {
  bool verified = verifyUser(chatID);
  Serial.println("DEBUG: New message: " + text);

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
      if(amountRegisteredUsers >= AMOUNT_USERS) {
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
      int thresholdNew = text.toInt();
      // No valid conversion
      if(thresholdNew == 0) {
        bot.sendMessage(chatID, "Ich bin kein Magier und kann aus Text keine Zahl machen. Vorgang abgebrochen.");
        waitingThreshold = "";
      }
      else {
        threshold = thresholdNew;
        writeMoistureThreshold(threshold);
        waitingThreshold = "";
        bot.sendMessage(chatID, "Neuer Schwellwert gesetzt.");
      }
    }
    else if(waitingReset == chatID) {
      bot.sendMessage(chatID, "War nett euch kennengelernt zu haben. Führe Factory Reset durch. Bitte trenne mich kurz vom Strom.");
      resetEEPROM();
    }

    else if(text == "/status") {
      int fillPercentage = (int)((tankFill*100)/TANK_VOLUME);
      bot.sendMessage(chatID, (String)"Die Tankfüllung liegt bei " + fillPercentage + "%.\nDer Messwert bei " + readMoisture(2) + ".");
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
      bot.sendMessage(chatID, (String)"Der Messwert des Sensors liegt bei " + readMoisture(2) + ".");
    }
    else if(text == "/setThreshold") {
      bot.sendMessage(chatID, (String)"Bitte gib den neuen Schwellwert für die Bewässerung ein. Der Schwellwert ist " + threshold + ", der Messwert liegt bei " + readMoisture(2) + ".");
      waitingThreshold = chatID;
    }
    else if(text == "/reset") {
      bot.sendMessage(chatID, "Hey, dir ist klar, dass du damit alle Einstellungen löschst? Bestätige den reset durch \"RESET\".");
      waitingReset = chatID;
    }
    else if(text == "/refill") {
      bot.sendMessage(chatID, (String)"Namnam, das schmeckt gut. Die Füllmenge liegt bei " + TANK_VOLUME + "ml.");
      tankFill = TANK_VOLUME;
      writeVolume(tankFill);
    }
    else {
      bot.sendMessage(chatID, "Der Befehl ist mit leider nicht bekannt.");
    }
  }
}

void notifyUsers() {
  Serial.println("DEBUG: Notifying users");
  for(int i=0; i<amountRegisteredUsers; i++) {
    bot.sendMessage(users[i], "Stille Wasser sind zwar tief, aber der Wasserstand im Tank wird langsam zu niedrig.");
  }
}



/**
 * 
 * User management
 * 
 */
boolean verifyUser(String chatID) {
  for(int i=0; i<amountRegisteredUsers; i++) {
    if(users[i] == chatID) {
      return true;
    }
  }
  return false;
}

void addUser(String chatID) {
  if(amountRegisteredUsers >= AMOUNT_USERS) return;
  writeChatID(amountRegisteredUsers, chatID);
  users[amountRegisteredUsers] = chatID;
  amountRegisteredUsers += 1;
  writeAmountRegisteredUsers(amountRegisteredUsers);
}

void deleteUser(String chatID) {
  if(amountRegisteredUsers == 0) return;
  String lastUser = users[amountRegisteredUsers-1];

  // find user to delete
  for(int i=0; i<amountRegisteredUsers; i++) {
    if(users[i] == chatID) {
      users[i] = lastUser;
      users[amountRegisteredUsers-1] = "";
      writeChatID(i, lastUser);
      writeChatID(amountRegisteredUsers-1, " ");
    }
  }
}
