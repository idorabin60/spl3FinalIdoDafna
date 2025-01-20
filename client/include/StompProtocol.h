#ifndef STOMPPROTOCOL_H
#define STOMPPROTOCOL_H

#include <string>
#include <unordered_map>
#include "StompFrame.h"

class StompProtocol
{
private:
    bool loggedIn;
    std::string username;
    std::unordered_map<std::string, std::string> subscriptions;

public:
    StompProtocol();

    // State Management
    bool isLoggedIn() const;
    void setLoggedIn(bool status);
    const std::string &getUsername() const;
    void setUsername(const std::string &user);

    // Command Processing
    StompFrame processCommand(const std::string &command);

    // Server Response Processing
    void processServerFrame(const StompFrame &frame);
};

#endif // STOMPPROTOCOL_H
