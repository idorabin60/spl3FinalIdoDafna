#pragma once
#include <string>
#include <iostream>
#include <map>
#include <vector>

// Declare the struct before any function or class that uses it
struct names_and_events
{
    std::string channel_name;
    std::vector<class Event> events; // Forward declaration of Event
};

// Event class declaration
class Event
{
private:
    // name of channel
    std::string channel_name;
    // city of the event
    std::string city;
    // name of the event
    std::string name;
    // time of the event in seconds
    int date_time;
    // description of the event
    std::string description;
    // map of all the general information
    std::map<std::string, std::string> general_information;
    std::string eventOwnerUser;

public:
    Event(std::string channel_name, std::string city, std::string name, int date_time, std::string description, std::map<std::string, std::string> general_information);
    Event(const std::string &frame_body);
    virtual ~Event();
    void setEventOwnerUser(std::string setEventOwnerUser);
    const std::string &getEventOwnerUser() const;
    const std::string &get_channel_name() const;
    const std::string &get_city() const;
    const std::string &get_description() const;
    const std::string &get_name() const;
    int get_date_time() const;
    const std::map<std::string, std::string> &get_general_information() const;
    void split_str(const std::string &str, char delimiter, std::vector<std::string> &result);
    std::string serialize() const;
    std::string trim(const std::string &s);
};

// Function to parse the JSON file and return a names_and_events object
names_and_events parseEventsFile(std::string json_path);

// Function to serialize the names_and_events struct into a string
std::string serializeNamesAndEvents(const names_and_events &data);
