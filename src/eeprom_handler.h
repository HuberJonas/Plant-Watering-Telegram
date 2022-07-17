#include <Arduino.h>
#include <EEPROM.h>

/**
 * EEPROM structure
 * 0-1  readingAir (value 1, 255)
 * 2-3  readingWater (value 1, 255)
 * 4    threshold
 * 5-6  volume (value 1, value 255)
 * 7    amount registered users
 * 
 * 10-24    ChatID User1
 * ...
 */


/**
 * 
 * EEPROM interface functions
 *
 */
int readNumberTwoParts(int index) {
    int first = EEPROM.read(index);
    int second = EEPROM.read(index+1);
    return first + second*256;
}

void writeNumberTwoParts(int index, int value) {
    int first = value%256;
    int second = int((value-first)/256);
    EEPROM.write(index, first);
    EEPROM.write(index+1, second);
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



void resetEEPROM() {
    for(unsigned int i=0; i<EEPROM.length(); i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}


/**
 *
 * Read/Write functions
 * 
 */
int readReadingAir() {
    return readNumberTwoParts(0);
}

void writeReadingAir(int value) {
    writeNumberTwoParts(0, value);
}



int readReadingWater() {
    return readNumberTwoParts(2);
}

void writeReadingWater(int value) {
    writeNumberTwoParts(2, value);
}



int readMoistureThreshold() {
    return EEPROM.read(4);
}

void writeMoistureThreshold(int value) {
    EEPROM.write(4, value);
    EEPROM.commit();
}



int readVolume() {
    return readNumberTwoParts(5);
}

void writeVolume(int value) {
    writeNumberTwoParts(5, value);
}



int readAmountRegisteredUsers() {
    return EEPROM.read(7);
}

void writeAmountRegisteredUsers(int amount) {
    EEPROM.write(7, amount);
    EEPROM.commit();
}



String readChatID(int index) {
    return readString(index*15+10);
}

void writeChatID(int index, String chatID) {
    writeString(index*15+10, chatID);
}
