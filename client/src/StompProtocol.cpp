#include "StompProtocol.h"
#include "event.h"
#include <iostream>
#include <sstream>

StompProtocol::StompProtocol() : loggedIn(false), username(""), reciptId(0), logOutId(-1) {}

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
int StompProtocol::getLogOutId()
{
    return logOutId;
}
void StompProtocol::setLogOutId(const int id)
{
    logOutId = id;
}
int StompProtocol::getReciptId()
{
    return reciptId;
}
void StompProtocol::setReciptId(const int id)
{
    reciptId = id;
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
        incremeantAndGetReciptId();
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
        setLogOutId(getReciptId());
        std::string reciptString = std::to_string(incremeantAndGetReciptId());
        frame.addHeader("receipt", reciptString);
    }

    return frame;
}
std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \n\r\t");
    size_t last = str.find_last_not_of(" \n\r\t");
    return str.substr(first, (last - first + 1));
}
#include <iostream>
#include <string>

std::string getReceiptId(const std::string &serializedFrame)
{
    const std::string key = "receipt-id:"; // The key to search for
    size_t startPos = serializedFrame.find(key);

    if (startPos == std::string::npos)
    {
        // Key not found
        return "";
    }

    // Move startPos to the position after "receipt-id:"
    startPos += key.length();

    // Find the end of the line
    size_t endPos = serializedFrame.find('\n', startPos);

    // Extract and return the substring
    return serializedFrame.substr(startPos, endPos - startPos);
}

void StompProtocol::processServerFrame(const StompFrame &frame)
{
    const std::string &command = frame.getCommand();

    // Use `find` to check if the command contains the desired keyword
    if (command.find("CONNECTED") != std::string::npos)
    {
        std::cout << "Login successful" << std::endl;
    }
    else if (command.find("MESSAGE") != std::string::npos)
    {
        std::cout << "Message from server recived " << std::endl;
    }
    else if (command.find("RECEIPT") != std::string::npos)
    {
        std::cout << "Receipt: " << getReceiptId(frame.serialize()) << std::endl;
        std::cout<<"log out id: "<< getLogOutId() << std::endl;
        if (std::stoi(getReceiptId(frame.serialize())) == getLogOutId())
        {
            setLoggedIn(false);
        }
    }
    else if (command.find("ERROR") != std::string::npos)
    {
        std::cerr << "Error: " << frame.getBody() << std::endl;
    }
    else
    {
        std::cerr << "Unknown command received: " << command << std::endl;
    }
}

std::vector<StompFrame> StompProtocol::processReportCommand(const std::string &filePath)
{
    std::vector<StompFrame> frames;

    try
    {
        names_and_events parsedData = parseEventsFile(filePath);

        for (const Event &event : parsedData.events)
        {
            StompFrame frame;
            frame.setCommand("SEND");

            // Add properly formatted headers
            frame.addHeader("destination", "/" + parsedData.channel_name);
            frame.addHeader("user", username);
            frame.addHeader("city", event.get_city());
            frame.addHeader("event name", event.get_name());
            frame.addHeader("date time", std::to_string(event.get_date_time()));

            // Format the general information
            std::ostringstream generalInfo;
            const auto &info = event.get_general_information();

            // Add general information fields
            if (info.find("active") != info.end())
            {
                generalInfo << "active: " << (info.at("active") == "true" ? "true" : "false") << "\n";
            }
            else
            {
                generalInfo << "active: false\n"; // Default if key is missing
            }

            if (info.find("forces_arrival_at_scene") != info.end())
            {
                generalInfo << "forces arrival at scene: " << (info.at("forces_arrival_at_scene") == "true" ? "true" : "false") << "\n";
            }
            else
            {
                generalInfo << "forces arrival at scene: false\n"; // Default if key is missing
            }

            // Add general information header
            frame.addHeader("general information", generalInfo.str());

            // Add the description as the body
            frame.setBody("description:\n" + event.get_description());

            // Add the frame to the vector
            frames.push_back(frame);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing report: " << e.what() << std::endl;
    }

    return frames;
}

int StompProtocol::incremeantAndGetReciptId()
{
    return reciptId++;
}
