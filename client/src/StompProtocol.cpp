#include "StompProtocol.h"
#include "event.h"
#include <iostream>
#include <sstream>

StompProtocol::StompProtocol() : loggedIn(false), username(""), reciptId(0) {}

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
        frame.addHeader("receipt", std::to_string(incremeantAndGetReciptId()));

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
        std::cout << "LogIn sucseesful" << std::endl;
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
std::vector<StompFrame> StompProtocol::processReportCommand(const std::string &filePath) {
    std::vector<StompFrame> frames;

    try {
        names_and_events parsedData = parseEventsFile(filePath);

        for (const Event &event : parsedData.events) {
            StompFrame frame;
            frame.setCommand("SEND");

            // Correct header formatting
            frame.addHeader("destination", " / " + parsedData.channel_name);
            frame.addHeader("user", " " + username);
            frame.addHeader("city", " " + event.get_city());
            frame.addHeader("event name", " " + event.get_name());
            frame.addHeader("date time", " " + std::to_string(event.get_date_time()));

            // General information as multiline
            std::ostringstream generalInfo;
            generalInfo << "active : " << (event.get_general_information().at("active") == "true" ? "true" : "false") << "\n";
            generalInfo << "forces arrival at scene : " << (event.get_general_information().at("forces arrival at scene") == "true" ? "true" : "false") << "\n";
            frame.addHeader("general information", "\n" + generalInfo.str());

            // Description as the body
            frame.setBody("description :\n" + event.get_description());

            frames.push_back(frame);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error processing report: " << e.what() << std::endl;
    }

    return frames;
}



int StompProtocol::incremeantAndGetReciptId()
{
    return reciptId++;
}
