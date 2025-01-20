#include "StompProtocol.h"
#include <iostream>
#include <sstream>

StompProtocol::StompProtocol() : loggedIn(false), username("") {}

bool StompProtocol::isLoggedIn() const
{
    return loggedIn;
}

void StompProtocol::setLoggedIn(bool status)
{
    loggedIn = status;
}

const std::string &StompProtocol::getUsername() const
{
    return username;
}

void StompProtocol::setUsername(const std::string &user)
{
    username = user;
}

StompFrame StompProtocol::processCommand(const std::string &command)
{
    std::istringstream iss(command);
    std::string action;
    iss >> action;

    StompFrame frame;

    if (action == "login")
    {
        std::string hostPort, username, password;
        iss >> hostPort >> username >> password;

        frame.setCommand("CONNECT");
        frame.addHeader("accept-version", "1.2");
        frame.addHeader("host", "stomp.cs.bgu.ac.il");
        frame.addHeader("login", username);
        frame.addHeader("passcode", password);

        setUsername(username);
    }
    else if (action == "join")
    {
        std::string channel;
        iss >> channel;

        std::string subscriptionId = std::to_string(subscriptions.size() + 1);
        frame.setCommand("SUBSCRIBE");
        frame.addHeader("destination", "/" + channel);
        frame.addHeader("id", subscriptionId);
        frame.addHeader("receipt", "join-" + subscriptionId);

        subscriptions[channel] = subscriptionId;
    }
    else if (action == "exit")
    {
        std::string channel;
        iss >> channel;

        auto it = subscriptions.find(channel);
        if (it != subscriptions.end())
        {
            frame.setCommand("UNSUBSCRIBE");
            frame.addHeader("id", it->second);
            frame.addHeader("receipt", "exit-" + it->second);

            subscriptions.erase(it);
        }
    }
    else if (action == "report")
    {
        std::string destination, messageBody;
        iss >> destination;
        std::getline(iss, messageBody);

        frame.setCommand("SEND");
        frame.addHeader("destination", "/" + destination);
        frame.setBody(messageBody);
    }
    else if (action == "logout")
    {
        frame.setCommand("DISCONNECT");
        frame.addHeader("receipt", "logout");
    }

    return frame;
}

void StompProtocol::processServerFrame(const StompFrame &frame)
{
    const std::string &command = frame.getCommand();

    if (command == "CONNECTED")
    {
        std::cout << "Connected to server. Version: " << frame.getHeader("version") << std::endl;
    }
    else if (command == "MESSAGE")
    {
        std::cout << "Message from " << frame.getHeader("destination") << ": " << frame.getBody() << std::endl;
    }
    else if (command == "RECEIPT")
    {
        std::cout << "Receipt: " << frame.getHeader("receipt-id") << std::endl;
    }
    else if (command == "ERROR")
    {
        std::cerr << "Error: " << frame.getBody() << std::endl;
    }
}
