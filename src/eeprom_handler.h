#include <Arduino.h>
#include <EEPROM.h>
#include "user/user.h"

/**
 * EEPROM structure
 * 0    wateringThreshold
 * 1    volume (first part 255^2)
 * 2    volume (second part 255^1)
 * 3    registered users
 * 
 * User 1
 * 10-14    Chat ID
 * 15-30    Name
 * ...
 */

int readThershold() {
    return EEPROM.read(0);
}

void writeThershold(int thershold) {
    EEPROM.write(0, threshold);
    EEPROM.commit();
}


int readVolume() {
    int first = EEPROM.read(1);
    int second = EEPROM.read(2);
    int volume = first*255 + second;
    return volume;
}

void writeVolume(int volume) {
    int second = volume%255;
    int first = (volume-second)/255;
    EEPROM.write(1, first);
    EEPROM.write(2, second);
    EEPROM.commit();
}


int readRegisteredUsers() {
    return EEPROM.read(3);
}

void writeRegisteredUsers(int amount) {
    EEPROM.write(3, amount);
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


// User readUser(int index) {
//     int startAddress = 10+20*index;
//     String chatID = readString(startAddress);
//     String name = readString(startAddress+5);
//     User user(name, chatID);
//     return user;
// }

// void writeUser(int index, User user) {
//     int startAddress = 10+20*index;
//     writeString(startAddress, user.getChatID());
//     writeString(startAddress+5, user.getName());
// }
