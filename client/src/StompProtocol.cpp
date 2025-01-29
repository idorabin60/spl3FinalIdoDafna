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
        if (subscriptions.find(channel) != subscriptions.end())
        {
            std::cerr << "Channel '" << channel << "' already joined.\n";
        }
        else
        {
            std::string subscriptionId = std::to_string(subscriptions.size() + 1);
            frame.setCommand("SUBSCRIBE");
            frame.addHeader("destination", "/" + channel);
            frame.addHeader("id", subscriptionId);
            frame.addHeader("receipt", std::to_string(incremeantAndGetReciptId()));
            subscriptions[channel] = subscriptionId;
        }
        return frame;
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
            std::cout << pair.first << " " << pair.second << '\n';
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
    int counter = 0;

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
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b)
              {
        if (a.get_date_time() == b.get_date_time())
            return a.get_name() < b.get_name();
        return a.get_date_time() < b.get_date_time(); });

    // Initialize statistics
    int totalReports = events.size();
    int activeCount = 0;
    int forcesArrivalCount = 0;
    int eventCounter = 1;

    std::ostringstream summaryStream;
    summaryStream << "Summary for Channel: " << channel_name << " | User: " << user << "\n";
    summaryStream << "----------------------------------------\n";

    for (const auto &event : events)
    {
        summaryStream << "Report_" << eventCounter++ << "\n";
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
    for (const auto &pair : eventMap)
    {
        std::cout << pair.first << std::endl;
    }
}

void StompProtocol::processMessageFinal(std::vector<std::string> messageParts)
{
    std::unordered_map<std::string, std::string> keyValueStore;

    // Parsing key-value pairs from input
    for (const std::string &segment : messageParts)
    {
        size_t delimiterPos = segment.find(':');
        if (delimiterPos != std::string::npos)
        {
            std::string keyField = segment.substr(0, delimiterPos);
            std::string valueField = segment.substr(delimiterPos + 1);
            keyValueStore[keyField] = valueField;
        }
    }

    // Ensuring all mandatory fields exist
    if (keyValueStore.count("user") == 0 || keyValueStore.count("destination") == 0 ||
        keyValueStore.count("event_name") == 0 || keyValueStore.count("city") == 0 ||
        keyValueStore.count("date_time") == 0 || keyValueStore.count("description") == 0 ||
        keyValueStore.count("general_information") == 0 || keyValueStore.count("active") == 0 ||
        keyValueStore.count("forces_arrival_at_scene") == 0)
    {
        std::cerr << "Error: Required message fields are missing." << std::endl;
        return;
    }

    // Extracting values from the parsed data
    std::string userID = keyValueStore["user"];
    std::string targetDestination = keyValueStore["destination"];
    std::string eventTitle = keyValueStore["event_name"];
    std::string eventCity = keyValueStore["city"];
    int eventTimestamp = std::stoi(keyValueStore["date_time"]);
    std::string eventDescription = keyValueStore["description"];
    std::string eventGeneralInfo = keyValueStore["general_information"];
    std::string eventStatus = keyValueStore["active"];
    std::string arrivalTimeInfo = keyValueStore["forces_arrival_at_scene"];

    // Storing general event-related details
    std::map<std::string, std::string> metadata;
    metadata["general_information"] = eventGeneralInfo;
    metadata["active"] = eventStatus;
    metadata["forces_arrival_at_scene"] = arrivalTimeInfo;

    // Creating and storing the event
    Event newEvent(targetDestination, eventCity, eventTitle, eventTimestamp, eventDescription, metadata);
    newEvent.setEventOwnerUser(userID);
    std::string channelKey = newEvent.get_channel_name();

    if (eventMap.find(channelKey) == eventMap.end())
    {
        eventMap[channelKey] = {};
    }
    eventMap[channelKey][userID].push_back(newEvent);
}

std::vector<std::string> StompProtocol::splitLine(const std::string &textLine)
{
    std::vector<std::string> tokens;
    std::string tokenBuffer = "";

    for (char currentChar : textLine)
    {
        if (currentChar == ' ' || currentChar == ':')
        {
            tokens.push_back(tokenBuffer);
            tokenBuffer = "";
        }
        else
        {
            tokenBuffer += currentChar;
        }
    }

    tokens.push_back(tokenBuffer);
    return tokens;
}

std::vector<std::string> StompProtocol::splitFrameToLines(const std::string &rawFrame)
{
    std::vector<std::string> extractedLines;
    std::string lineBuffer = "";

    for (char frameChar : rawFrame)
    {
        if (frameChar == '\0')
        {
            return extractedLines;
        }
        if (frameChar == '\n')
        {
            extractedLines.push_back(lineBuffer);
            lineBuffer = "";
        }
        else
        {
            lineBuffer += frameChar;
        }
    }

    // Append the last collected line if it's not empty
    if (!lineBuffer.empty())
    {
        extractedLines.push_back(lineBuffer);
    }

    return extractedLines;
}
