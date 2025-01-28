#include "StompProtocol.h"
#include "event.h"
#include <iostream>
#include <sstream>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <mutex>

// Declare global variables from the shared file
extern std::condition_variable cv;
extern std::mutex mtx;
extern std::atomic<bool> receiptProcessed;
StompProtocol::StompProtocol()
    : loggedIn(false), username(""), reciptId(0), logOutId(-1), subscriptions(), eventMap(), isError(false) {}
bool StompProtocol::isLoggedIn() const
{
    return loggedIn;
}
bool StompProtocol::getIsError()
{
    return isError;
}
void StompProtocol::setIsError(bool status)
{
    isError = status;
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
            frame.addHeader("receipt", std::to_string(incremeantAndGetReciptId()));

            subscriptions.erase(it);
        }
        else
        {
            std::cerr << "Channel '" << channel << "' not found for this user.\n";
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
    } // bla

    // Move startPos to the position after "receipt-id:"
    startPos += key.length();

    // Find the end of the line
    size_t endPos = serializedFrame.find('\n', startPos);

    // Extract and return the substring
    return serializedFrame.substr(startPos, endPos - startPos);
}
void StompProtocol::processServerFrame(const std::string &serverMessage)
{
    std::istringstream stream(serverMessage);
    std::string line, command;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    // Parse the command (first line of the message)
    if (std::getline(stream, line))
    {
        command = line;
    }

    // Parse headers
    while (std::getline(stream, line) && !line.empty())
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));
            headers[key] = value;
        }
    }

    // Parse body
    std::ostringstream bodyStream;
    while (std::getline(stream, line))
    {
        bodyStream << line << "\n";
    }
    body = bodyStream.str();
    // std::cout << "THIS IS THE BODY!" << body << std::endl;

    if (command == "MESSAGE")
    {
        handleMessage(serverMessage);
    }

    else if (command == "RECEIPT")
    {
        std::string receiptId = headers["receipt-id"];
        std::cout << "Receipt ID: " << receiptId << std::endl;
        handleRecipt(receiptId);
    }
    else if (command == "CONNECTED")
    {
        setLoggedIn(true);
    }

    else if (command == "ERROR")
    {
        setIsError(true);
        for (const auto &pair : headers)
        {
            std::cout  << pair.first << " " << pair.second << '\n';
        }
        std::cerr << body << "\n";
    }
    else
    {
        std::cerr << "Unknown command received: " << command << "\n";
    }
}

std::vector<StompFrame> StompProtocol::processReportCommand(const std::string &filePath)
{
    int counter =0;

    try
    {
        names_and_events parsedData = parseEventsFile(filePath);
        std::vector<StompFrame> frames;

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
            frame.setBody("description:\n" + event.get_description());

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

            // Add the frame to the vector
            frames.push_back(frame);
            counter++;
        }
        std::cout<<"LOOK HERE YA HOMO"<<counter<<std::endl;
        return frames;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing report: " << e.what() << std::endl;
    }
}

int StompProtocol::incremeantAndGetReciptId()
{
    return reciptId++;
}
void StompProtocol::reset()
{
    loggedIn = false;
    username.clear();
    reciptId = 0;
    logOutId = -1;
    subscriptions.clear();         // Clear all active subscriptions
    receiptProcessed.store(false); // Reset the receiptProcessed flag
    eventMap.clear();
    setIsError(false); // Clear stored events
}

void StompProtocol::summarize(const std::string &channel_name, const std::string &user, const std::string &file) const
{
    // Check if the channel exists
    auto channelIt = eventMap.find(channel_name);
    if (channelIt == eventMap.end())
    {
        std::cerr << "Channel '" << channel_name << "' not found.\n";
        return;
    }

    // Check if the user exists within the channel
    auto userIt = channelIt->second.find(user);
    if (userIt == channelIt->second.end())
    {
        std::cerr << "User '" << user << "' not found in channel '" << channel_name << "'.\n";
        return;
    }

    auto events = userIt->second;

    // Sort events by date_time, then by event_name lexicographically
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        if (a.get_date_time() == b.get_date_time())
            return a.get_name() < b.get_name();
        return a.get_date_time() < b.get_date_time();
    });

    // Initialize statistics
    int totalReports = events.size();
    int activeCount = 0;
    int forcesArrivalCount = 0;

    std::ostringstream summaryStream;
    summaryStream << "Summary for Channel: " << channel_name << " | User: " << user << "\n";
    summaryStream << "----------------------------------------\n";

    for (const auto &event : events)
    {
        // Count active and forces arrival flags
        const auto &generalInfo = event.get_general_information();
        if (generalInfo.count("active") && generalInfo.at("active") == "true")
        {
            activeCount++;
        }
        if (generalInfo.count("forces_arrival_at_scene") && generalInfo.at("forces_arrival_at_scene") == "true")
        {
            forcesArrivalCount++;
        }

        // Format date and time
        std::time_t eventTime = static_cast<time_t>(event.get_date_time());
        std::tm *timeInfo = std::gmtime(&eventTime);
        std::ostringstream dateTimeStream;
        dateTimeStream << std::put_time(timeInfo, "%d/%m/%y %H:%M");

        // Generate summary description
        std::string description = event.get_description();
        if (description.size() > 27)
        {
            description = description.substr(0, 27) + "...";
        }

        // Add event details to the summary
        summaryStream << "Event: " << event.get_name() << "\n";
        summaryStream << "  City: " << event.get_city() << "\n";
        summaryStream << "  Date: " << dateTimeStream.str() << "\n";
        summaryStream << "  Summary: " << description << "\n";
        summaryStream << "----------------------------------------\n";
    }

    // Add statistics to the summary
    summaryStream << "Stats:\n";
    summaryStream << "Total: " << totalReports << "\n";
    summaryStream << "Active: " << activeCount << "\n";
    summaryStream << "Forces Arrival: " << forcesArrivalCount << "\n";

    // Write the summary to a file
    std::ofstream outFile(file);
    if (!outFile)
    {
        std::cerr << "Unable to write summary to file '" << file << "'.\n";
        return;
    }

    outFile << summaryStream.str();
    outFile.close();

    std::cout << "Summary successfully written to '" << file << "'.\n";
}

void StompProtocol::handleMessage(std::string serverMessage)
{
    std::lock_guard<std::mutex> lock(eventMapMutex);
    std::vector<std::string> args = splitFrameToLines(serverMessage);
    processMessageFinal(args);
    
    // Event event(serverMessage);
    // std::lock_guard<std::mutex> lock(eventMapMutex);
    // std::string chanelName = event.get_channel_name();
    // chanelName.erase(0, 2);
    // std::string user = event.getEventOwnerUser();
    // user.erase(0,1);

    // eventMap[chanelName][user].push_back(event);
    // std::cout << event.toString();
}
void StompProtocol::handleRecipt(std::string receiptId)
{
    if (!receiptId.empty() && std::stoi(receiptId) == getLogOutId())
    {
        setLoggedIn(false);
    }
}
void StompProtocol::printEventMap()
{
   std::cout << "Top-level keys in eventMap:" << std::endl;
    for (const auto& pair : eventMap) {
        std::cout << pair.first << std::endl;
    }
}

void StompProtocol::processMessageFinal(std::vector<std::string> args) {
    std::unordered_map<std::string, std::string> argMap;
    for (const std::string& arg : args) {
        size_t pos = arg.find(':');
        if (pos != std::string::npos) {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            argMap[key] = value;
        }
    }

    // Check if all required keys exist in the map
    if (argMap.find("user") == argMap.end() ||
        argMap.find("destination") == argMap.end() ||
        argMap.find("event_name") == argMap.end() ||
        argMap.find("city") == argMap.end() ||
        argMap.find("date_time") == argMap.end() ||
        argMap.find("description") == argMap.end() ||
        argMap.find("general_information") == argMap.end() ||
        argMap.find("active") == argMap.end() ||
        argMap.find("forces_arrival_at_scene") == argMap.end()) {
        std::cerr << "Error: Missing required fields in the message" << std::endl;
        return;
    }

    std::string user = argMap["user"];
    std::string destination = argMap["destination"];
    std::string event_name = argMap["event_name"];
    std::string city = argMap["city"];
    int date_time = std::stoi(argMap["date_time"]);
    std::string description = argMap["description"];
    std::string general_information = argMap["general_information"];
    std::string active = argMap["active"];
    std::string forces_arrival_at_scene = argMap["forces_arrival_at_scene"];

    std::map<std::string, std::string> general_info_map;
    general_info_map["general_information"] = general_information;
    general_info_map["active"] = active;
    general_info_map["forces_arrival_at_scene"] = forces_arrival_at_scene;

    Event event(destination, city, event_name, date_time, description, general_info_map);
    event.setEventOwnerUser(user);
    std::string channel = event.get_channel_name();
    if (eventMap.find(channel) == eventMap.end()) {
        eventMap[channel] = {};
    }
    eventMap[channel][user].push_back(event);
}

std::vector<std::string> StompProtocol :: splitLine(const std::string& line) {
    std::vector<std::string> args = {};
    std::string word = "";
    for(char c : line) {
        if(c == ' ' or c == ':') {
            args.push_back(word);
            word = "";
        }
        else {
            word += c;
        }
    }
    args.push_back(word);
    return args;
}

std::vector<std::string> StompProtocol::splitFrameToLines(const std::string& frame) {
    std::vector<std::string> args;
    std::string line = "";
    for(char c : frame) {
        if(c == '\0') {
            return args;
        }
        if(c == '\n') {
            args.push_back(line);
            line = "";
        }
        else {
            line += c;
        }
    }
    // Handle the case where the frame does not end with a newline
    if (!line.empty()) {
        args.push_back(line);
    }
    return args;
}

