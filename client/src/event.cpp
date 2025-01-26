#include "../include/event.h"
#include "../include/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <locale>
using namespace std;
using json = nlohmann::json;

// Constructor
Event::Event(std::string channel_name, std::string city, std::string name, int date_time,
             std::string description, std::map<std::string, std::string> general_information)
    : channel_name(channel_name), city(city), name(name),
      date_time(date_time), description(description), general_information(general_information), eventOwnerUser("") {}

// Destructor
Event::~Event() {}

// Split string by delimiter
void Event::split_str(const std::string &str, char delimiter, std::vector<std::string> &result)
{
    result.clear();
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        result.push_back(item);
    }
}

// Set the event owner user
void Event::setEventOwnerUser(std::string setEventOwnerUser)
{
    eventOwnerUser = setEventOwnerUser;
}

// Getters
const std::string &Event::getEventOwnerUser() const { return eventOwnerUser; }
const std::string &Event::get_channel_name() const { return channel_name; }
const std::string &Event::get_city() const { return city; }
const std::string &Event::get_name() const { return name; }
int Event::get_date_time() const { return date_time; }
const std::map<std::string, std::string> &Event::get_general_information() const { return general_information; }
const std::string &Event::get_description() const { return description; }

// Constructor to parse event from a frame body
std::string Event::trim(const std::string &s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start))
    {
        start++;
    }

    auto end = s.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}
Event::Event(const std::string &frame_body)
    : channel_name(""), city(""), name(""), date_time(0), description(""), general_information(), eventOwnerUser("")
{
    std::istringstream stream(frame_body);
    std::string line;

    while (std::getline(stream, line))
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));

            if (key == "user")
                eventOwnerUser = value;
            else if (key == "event name")
                name = value;
            else if (key == "city")
                city = value;
            else if (key == "date time")
                date_time = std::stoi(value);
            else if (key == "general information")
            {
                // Process subsequent lines as general information
                while (std::getline(stream, line) && line.find(':') != std::string::npos)
                {
                    size_t infoColonPos = line.find(':');
                    std::string infoKey = trim(line.substr(0, infoColonPos));
                    std::string infoValue = trim(line.substr(infoColonPos + 1));
                    general_information[infoKey] = infoValue;
                }
            }
            else if (key == "description")
            {
                // Process the remaining lines as the description
                std::ostringstream descStream;
                descStream << value << "\n";
                while (std::getline(stream, line))
                {
                    descStream << line << "\n";
                }
                description = descStream.str();
                break; // End parsing after the description
            }
        }
    }
}

// Parse JSON file into a names_and_events object
names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string channel_name = data["channel_name"];
    std::vector<Event> events;

    for (auto &event : data["events"])
    {
        std::string name = event["event_name"];
        std::string city = event["city"];
        int date_time = event["date_time"];
        std::string description = event["description"];
        std::map<std::string, std::string> general_information;

        for (auto &update : event["general_information"].items())
        {
            if (update.value().is_string())
                general_information[update.key()] = update.value();
            else
                general_information[update.key()] = update.value().dump();
        }

        events.emplace_back(channel_name, city, name, date_time, description, general_information);
    }

    return {channel_name, events};
}

// Serialize a names_and_events object to JSON
std::string serializeNamesAndEvents(const names_and_events &data)
{
    std::ostringstream oss;
    oss << "{ \"channel_name\": \"" << data.channel_name << "\", \"events\": [";

    for (size_t i = 0; i < data.events.size(); ++i)
    {
        const Event &event = data.events[i];
        oss << event.serialize();
        if (i < data.events.size() - 1)
        {
            oss << ", ";
        }
    }

    oss << "]}";
    return oss.str();
}

// Serialize an individual Event
std::string Event::serialize() const
{
    std::ostringstream oss;
    oss << "{"
        << "\"name\": \"" << get_name() << "\", "
        << "\"city\": \"" << get_city() << "\", "
        << "\"date_time\": " << get_date_time() << ", "
        << "\"description\": \"" << get_description() << "\", "
        << "\"general_information\": {";

    const auto &general_info = get_general_information();
    for (auto it = general_info.begin(); it != general_info.end(); ++it)
    {
        if (it != general_info.begin())
        {
            oss << ", ";
        }
        oss << "\"" << it->first << "\": \"" << it->second << "\"";
    }

    oss << "}"
        << "}";
    return oss.str();
}
