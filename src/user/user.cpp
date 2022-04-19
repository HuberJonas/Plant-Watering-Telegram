#include <Arduino.h>
#include "user.h"

User::User() {
    name = "noName";
    chatID = "0";
}

User::User(String _name, String _chatID) {
    name = _name;
    chatID = _chatID;
}

String User::getName() {
    return name;
}

String User::getChatID() {
    return chatID;
}
