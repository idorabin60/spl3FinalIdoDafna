#ifndef STOMPPROTOCOL_H
#define STOMPPROTOCOL_H

#include <string>
#include <unordered_map>
#include <vector> // Required for std::vector
#include "StompFrame.h"
#include "event.h"
#include <mutex>
class StompProtocol
{
private:
    bool loggedIn;
    std::string username;
    std::unordered_map<std::string, std::string> subscriptions;
    int reciptId;
    int logOutId;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Event>>> eventMap;
    std::mutex eventMapMutex;

public:
    StompProtocol();

    // State Management
    bool isLoggedIn() const;
    void setLoggedIn(bool status);
    const std::string &getUsername() const;
    void setUsername(const std::string &user);
    void setLogOutId(const int logOutId);
    int getLogOutId();
    int getReciptId();
    void setReciptId(const int id);

    // Command Processing
    StompFrame processCommand(const std::string &command);
    std::vector<StompFrame> processReportCommand(const std::string &filePath); // Fixed declaration

    // Server Response Processing
    void processServerFrame(const std::string &serverMessage);
    int incremeantAndGetReciptId();
    void reset();
    void summarize(const std::string &channel_name, const std::string &user, const std::string &file) const;
    void printEventMap() const;
};

#endif // STOMPPROTOCOL_H
