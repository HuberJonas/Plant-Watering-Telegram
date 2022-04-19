#if !defined(User_h)
#define User_h

#include <Arduino.h>

class User {
    private:
        String name;
        String chatID;
    
    public:
        User();
        User(String _name, String _chatID);
        String getName();
        String getChatID();
};

#endif // User_h
