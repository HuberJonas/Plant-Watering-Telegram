#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "settings.h"
#include "eeprom_handler.h"

void checkMoisture();
void waterPlant();
int readMoisture(int amountReadings);
int readSensorRaw(int amountReadings);
void updateBot();
void handleMessage(String chatID, String text, String name);
void notifyUsers(String message);
boolean verifyUser(String chatID);
void addUser(String chatID);
void deleteUser(String chatID);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure clientSecure;
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);
boolean usersHaveBeenNotifiedTank = false;
boolean usersHaveBeenNotifiedTankEmpty = false;
boolean usersHaveBeenNotifiedCalibration = false;
unsigned long lastMillisUpdateBot;
unsigned long lastMillisCheckMoisture;
unsigned long lastMillisWatering;

int threshold = 0;
int readingAir = 0;
int readingWater = 0;
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
  delay(100);

  // Pump and EEPROM
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  EEPROM.begin(AMOUNT_USERS*15+10);

  threshold = readMoistureThreshold();
  readingAir = readReadingAir();
  readingWater = readReadingWater();
  tankFill = readVolume();
  Serial.println("DEBUG: Started EEPROM");

  amountRegisteredUsers = readAmountRegisteredUsers();
  for(int i=0; i<amountRegisteredUsers; i++) {
    users[i] = readChatID(i);
  }
  Serial.println("Ready to be used!");
}


void loop() {
  if(millis() - lastMillisUpdateBot >= UPDATE_INTERVAL) {
    updateBot();
    lastMillisUpdateBot = millis();
  }

  if(millis() - lastMillisCheckMoisture >= 5000) {
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
  if(reading < threshold && millis() - lastMillisWatering >= PUMP_WAIT) {
    waterPlant();
  }

  // check if water level is critical
  if(tankFill < TANK_VOLUME*0.1 && usersHaveBeenNotifiedTank == false) {
    Serial.println("Critical tank fill reached.");
    notifyUsers("Stille Wasser sind zwar tief, aber der Wasserstand im Tank wird langsam zu niedrig.");
    usersHaveBeenNotifiedTank = true;
  }
  else if (tankFill > TANK_VOLUME*0.1) {
    usersHaveBeenNotifiedTank = false;
  }
}


void waterPlant() {
  if(tankFill < PUMP_VOLUME) {
    Serial.println("ERROR: Tank is completely empty");
    if(!usersHaveBeenNotifiedTankEmpty) {
      notifyUsers("Nein, meine Suppe ess ich nicht. Aber ich will Wasser, denn der Tank ist leer.");
      usersHaveBeenNotifiedTankEmpty = true;
    }
    return;
  }
  usersHaveBeenNotifiedTankEmpty = false;
  Serial.println("Watering the plant");
  digitalWrite(PUMP_PIN, HIGH);
  delay(PUMP_INTERVAL);
  digitalWrite(PUMP_PIN, LOW);
  tankFill -= PUMP_VOLUME;
  writeVolume(tankFill);
  lastMillisWatering = millis();
}


int readMoisture(int amountReadings) {
  // read value
  int reading = 0;
  for (int i = 0; i < amountReadings; i++) {
    reading += analogRead(SENSOR_PIN);
    delay(50);
  }
  reading = (int)reading/amountReadings;

  if(readingAir == 0 && readingWater == 0 || readingAir == readingWater) {
    Serial.println("ERROR: Sensor has to be calibrated!");
    if(amountRegisteredUsers != 0 && !usersHaveBeenNotifiedCalibration) {
      notifyUsers("Hey, bitte kalibriere den Sensor, indem du /calibrate schickst. Sonst kann ich leider nicht arbeiten.");
      usersHaveBeenNotifiedCalibration = true;
    }
    return 100;
  }
  else {
    usersHaveBeenNotifiedCalibration = false;
  }
  return map(reading, min(readingAir, readingWater), max(readingAir, readingWater), 0, 100);
}

int readSensorRaw(int amountReadings) {
  // read value
  int reading = 0;
  for (int i = 0; i < amountReadings; i++) {
    reading += analogRead(SENSOR_PIN);
    delay(50);
  }
  return (int)reading/amountReadings;
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
String waitingCalibration = "";

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
        deleteUser(chatID);
      }
      else{
        bot.sendMessage(chatID, "Vorgang abgebrochen.");
      }
      waitingDeletion = "";
    }
    else if(waitingThreshold == chatID) {
      int thresholdNew = text.toInt();
      // No valid conversion
      if(thresholdNew == 0) {
        bot.sendMessage(chatID, "Ich bin kein Magier und kann aus Text keine Zahl machen. Vorgang abgebrochen.");
      }
      else {
        threshold = thresholdNew;
        writeMoistureThreshold(threshold);
        bot.sendMessage(chatID, "Neuer Schwellwert gesetzt.");
      }
      waitingThreshold == "";
    }
    else if(waitingReset == chatID) {
      if(text == "RESET") {        
        bot.sendMessage(chatID, "War nett euch kennengelernt zu haben. Führe Factory Reset durch. Bitte trenne mich kurz vom Strom.");
        resetEEPROM();
      }
      else {
        bot.sendMessage(chatID, "Vorgang abgebrochen");
      }
      waitingReset = "";
    }
    else if(waitingCalibration == chatID) {
      if(text == "CALIBRATE") {
        Serial.println("DEBUG: Sensor calibration started");
        bot.sendMessage(chatID, "Cool, lasset die Kalibrierung beginnen. Bitte halte dafür den Sensor an die Luft. Stelle sicher, dass dieser trocken ist. Ich melde mich in 20s wieder");
        delay(20000);
        readingAir = readSensorRaw(10);
        Serial.println((String)"DEBUG: readingAir: " + readingAir);
        writeReadingAir(readingAir);
        bot.sendMessage(chatID, "Super. Stelle den Sensor nun bitte in ein Glas mit Wasser. Ich melde mich in 20s wieder");
        delay(20000);
        readingWater = readSensorRaw(10);
        Serial.println((String)"DEBUG: readingWater: " + readingWater);
        writeReadingWater(readingWater);
        bot.sendMessage(chatID, "Perfekt. Kalibrierung abgeschlossen");
      }
      else {
        bot.sendMessage(chatID, "Vorgang abgebrochen");
      }
      waitingCalibration = "";
    }

    else if(text == "/status") {
      int fillPercentage = (int)((tankFill*100)/TANK_VOLUME);
      bot.sendMessage(chatID, (String)"Tankfüllung: " + fillPercentage + "%.\nFeuchtigkeit: " + readMoisture(2) + "%.\nSchwellwert: " + threshold + "%");
    }
    else if(text == "/secret") {
      bot.sendMessage(chatID, "Hier kommt der Schlüssel. Teile ihn nicht mit jedem!");
      bot.sendMessage(chatID, SECRET);
    }
    else if(text == "/deleteme") {
      bot.sendMessage(chatID, "ACHTUNG\nBestätige das Löschen, indem du \"Delete\" schickst.");
      waitingDeletion = chatID;
    }
    else if(text == "/water") {
      bot.sendMessage(chatID, "Wie du befiehlst. Ich gieße!");
      waterPlant();
    }
    else if(text == "/setthreshold") {
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
    else if(text == "/calibrate") {
      bot.sendMessage(chatID, (String)"Für die Kalibrierung benötigst du ein Gefäß mit Wasser. Hole eins und bestätige die Durchführung der Kalibrierung, indem du \"CALIBRATE\" schickst.");
      bot.sendMessage(chatID, (String)"Die aktuellen Werte liegen bei:\nMesswert Luft: " + readingAir + "\nMesswert Wasser: " + readingWater);
      waitingCalibration = chatID;
    }
    else {
      bot.sendMessage(chatID, "Der Befehl ist mit leider nicht bekannt.");
    }
  }
}

void notifyUsers(String message) {
  Serial.println("DEBUG: Notifying users");
  for(int i=0; i<amountRegisteredUsers; i++) {
    bot.sendMessage(users[i], message);
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
