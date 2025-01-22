#ifndef STOMPFRAME_H
#define STOMPFRAME_H

#include <string>
#include <map>
#include <sstream>

class StompFrame
{
private:
    std::string command;                        // The STOMP command (e.g., CONNECT, SUBSCRIBE, etc.)
    std::map<std::string, std::string> headers; // Frame headers in key-value format
    std::string body;                           // The body content of the frame

public:
    // Constructors
    StompFrame(const std::string &cmd = ""); // Default constructor with optional command

    // Getters and Setters
    void setCommand(const std::string &cmd);
    std::string getCommand() const;

    void addHeader(const std::string &key, const std::string &value);
    std::string getHeader(const std::string &key) const;

    void setBody(const std::string &body);
    std::string getBody() const;

    // Serialization and Parsing
    std::string serialize() const;
    std::string serialize2() const;                       // Convert the frame to a string for transmission
                                                          // Convert the frame to a string for transmission
    static StompFrame parse(const std::string &rawFrame); // Parse a raw frame string into a StompFrame object
};

#endif // STOMPFRAME_H
