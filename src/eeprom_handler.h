#include <Arduino.h>
#include <EEPROM.h>

/**
 * EEPROM structure
 * 0    moistureThreshold (first part   value: 1)
 * 1    moistureThreshold (second part  value: 255)
 * 2    volume (first part      value: 1)
 * 3    volume (second part     value: 255)
 * 4    amount registered users
 * 
 * 10-24    ChatID User1
 * ...
 */

int readMoistureThreshold() {
    int first = EEPROM.read(0);
    int second = EEPROM.read(1);
    int threshold = first + second*256;
    return threshold;
}

void writeMoistureThreshold(int threshold) {
    int first = threshold%256;
    int second = int((threshold-first)/256);
    EEPROM.write(0, first);
    EEPROM.write(1, second);
    EEPROM.commit();
}


int readVolume() {
    int first = EEPROM.read(2);
    int second = EEPROM.read(3);
    int volume = first + second*256;
    return volume;
}

void writeVolume(int volume) {
    int first = volume%256;
    int second = (volume-first)/256;
    EEPROM.write(2, first);
    EEPROM.write(3, second);
    EEPROM.commit();
}


int readAmountRegisteredUsers() {
    return EEPROM.read(4);
}

void writeAmountRegisteredUsers(int amount) {
    EEPROM.write(4, amount);
    EEPROM.commit();
}


String readString(int address) {
    int len = EEPROM.read(address);
    char content[len+1];
    for(int i=0; i<len; i++) {
        content[i] = EEPROM.read(address+1+i);
    }
    content[len] = '\0';
    return (String)content;
}

void writeString(int address, String content) {
    int len = content.length();
    if(len > 9) return;
    EEPROM.write(address, len);
    for(int i=0; i<len; i++) {
        EEPROM.write(address+1+i, content[i]);
    }
    EEPROM.commit();
}


String readChatID(int index) {
    return readString(index*15+10);
}

void writeChatID(int index, String chatID) {
    writeString(index*15+10, chatID);
}


void resetEEPROM() {
    for(int i=0; i<EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}
