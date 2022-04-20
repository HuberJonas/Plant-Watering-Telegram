#include <Arduino.h>
#include <EEPROM.h>
#include "user/user.h"

/**
 * EEPROM structure
 * 0    wateringThreshold (first part)
 * 1    wateringThreshold (second part)
 * 2    volume (first part 255^2)
 * 3    volume (second part 255^1)
 * 4    registered users
 * 
 * 10-14    ChatID User1
 * ...
 */

int readThershold() {
    int first = EEPROM.read(0);
    int second = EEPROM.read(1);
    int threshold = first*255 + second;
    return threshold;
}

void writeThershold(int thershold) {
    int second = volume%255;
    int first = (volume-second)/255;
    EEPROM.write(0, first);
    EEPROM.write(1, second);
    EEPROM.commit();
}


int readVolume() {
    int first = EEPROM.read(2);
    int second = EEPROM.read(3);
    int volume = first*255 + second;
    return volume;
}

void writeVolume(int volume) {
    int second = volume%255;
    int first = (volume-second)/255;
    EEPROM.write(2, first);
    EEPROM.write(3, second);
    EEPROM.commit();
}


int readRegisteredUsers() {
    return EEPROM.read(4);
}

void writeRegisteredUsers(int amount) {
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
    for(int i=0, i<len; i++) {
        EEPROM.write(address+1+i, content[i]);
    }
    EEPROM.commit();
}


String readChatID(int index) {
    return readString(index*5+10);
}

void writeChatID(int index, String chatID) {
    writeString(index*5+10, chatID);
}
