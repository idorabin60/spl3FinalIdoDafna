#include "../include/event.h"
#include "../include/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>

// #include "../include/keyboardInput.h"
#include "event.h"

using namespace std;
using json = nlohmann::json;

Event::Event(std::string channel_name, std::string city, std::string name, int date_time,
             std::string description, std::map<std::string, std::string> general_information)
    : channel_name(channel_name), city(city), name(name),
      date_time(date_time), description(description), general_information(general_information), eventOwnerUser("")
{
}

Event::~Event()
{
}

// Member function for splitting a string
void Event::split_str(const std::string &str, char delimiter, std::vector<std::string> &result)
{
    result.clear();            // Clear the result vector to avoid appending to existing data
    std::stringstream ss(str); // Use a stringstream to process the string
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        result.push_back(item); // Add each substring to the result vector
    }
}

void Event::setEventOwnerUser(std::string setEventOwnerUser)
{
    eventOwnerUser = setEventOwnerUser;
}

const std::string &Event::getEventOwnerUser() const
{
    return eventOwnerUser;
}

const std::string &Event::get_channel_name() const
{
    return this->channel_name;
}

const std::string &Event::get_city() const
{
    return this->city;
}

const std::string &Event::get_name() const
{
    return this->name;
}

int Event::get_date_time() const
{
    return this->date_time;
}

const std::map<std::string, std::string> &Event::get_general_information() const
{
    return this->general_information;
}

const std::string &Event::get_description() const
{
    return this->description;
}

Event::Event(const std::string &frame_body) : channel_name(""), city(""),
                                              name(""), date_time(0), description(""), general_information(),
                                              eventOwnerUser("")
{
    stringstream ss(frame_body);
    string line;
    string eventDescription;
    map<string, string> general_information_from_string;
    bool inGeneralInformation = false;
    while (getline(ss, line, '\n'))
    {
        vector<string> lineArgs;
        if (line.find(':') != string::npos)
        {
            split_str(line, ':', lineArgs);
            string key = lineArgs.at(0);
            string val;
            if (lineArgs.size() == 2)
            {
                val = lineArgs.at(1);
            }
            if (key == "user")
            {
                eventOwnerUser = val;
            }
            if (key == "channel name")
            {
                channel_name = val;
            }
            if (key == "city")
            {
                city = val;
            }
            else if (key == "event name")
            {
                name = val;
            }
            else if (key == "date time")
            {
                date_time = std::stoi(val);
            }
            else if (key == "general information")
            {
                inGeneralInformation = true;
                continue;
            }
            else if (key == "description")
            {
                while (getline(ss, line, '\n'))
                {
                    eventDescription += line + "\n";
                }
                description = eventDescription;
            }

            if (inGeneralInformation)
            {
                general_information_from_string[key.substr(1)] = val;
            }
        }
    }
    general_information = general_information_from_string;
}

names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string channel_name = data["channel_name"];

    // run over all the events and convert them to Event objects
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

        events.push_back(Event(channel_name, city, name, date_time, description, general_information));
    }
    names_and_events events_and_names{channel_name, events};

    return events_and_names;
}
std::string serializeNamesAndEvents(const names_and_events &data)
{
    std::ostringstream oss;
    oss << "{ \"channel_name\": \"" << data.channel_name << "\", \"events\": [";

    for (size_t i = 0; i < data.events.size(); ++i)
    {
        const Event &event = data.events[i];
        oss << "{"
            << "\"name\": \"" << event.get_name() << "\", "
            << "\"city\": \"" << event.get_city() << "\", "
            << "\"date_time\": " << event.get_date_time() << ", "
            << "\"description\": \"" << event.get_description() << "\", "
            << "\"general_information\": {";

        const auto &general_info = event.get_general_information();
        for (auto it = general_info.begin(); it != general_info.end(); ++it)
        {
            if (it != general_info.begin())
            {
                oss << ", ";
            }
            oss << "\"" << it->first << "\": \"" << it->second << "\"";
        }

        oss << "}}";

        if (i < data.events.size() - 1)
        {
            oss << ", ";
        }
    }

    oss << "]}";

    return oss.str();
}
std::string Event::toString() const
{
    std::ostringstream oss;

    oss << "Event Details:\n";
    oss << "Channel Name: " << channel_name << "\n";
    oss << "City: " << city << "\n";
    oss << "Event Name: " << name << "\n";
    oss << "Date Time: " << date_time << "\n";
    oss << "Description: " << description << "\n";
    oss << "Event Owner User: " << eventOwnerUser << "\n";

    oss << "General Information:\n";
    for (const auto &info : general_information)
    {
        oss << "  " << info.first << ": " << info.second << "\n";
    }

    return oss.str();
}
