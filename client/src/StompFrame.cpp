#include "../include/StompFrame.h"

// Constructor
StompFrame::StompFrame(const std::string &cmd) : command(cmd) {}

// Set the command
void StompFrame::setCommand(const std::string &cmd)
{
    command = cmd;
}

// Get the command
std::string StompFrame::getCommand() const
{
    return command;
}

// Add a header to the frame
void StompFrame::addHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

// Get the value of a header
std::string StompFrame::getHeader(const std::string &key) const
{
    auto it = headers.find(key);
    return (it != headers.end()) ? it->second : "";
}

// Set the body content
void StompFrame::setBody(const std::string &body)
{
    this->body = body;
}

// Get the body content
std::string StompFrame::getBody() const
{
    return body;
}

// Serialize the frame into a string for transmission
std::string StompFrame::serialize() const
{
    std::ostringstream oss;

    // Add the command
    oss << command << "\n";

    // Add the headers
    for (const auto &header : headers)
    {
        oss << header.first << ":" << header.second << "\n";
    }

    // Add a blank line and then the body
    oss << "\n"
        << body << "\0";

    return oss.str();
}

// Parse a raw STOMP frame string into a StompFrame object
StompFrame StompFrame::parse(const std::string &rawFrame)
{
    std::istringstream iss(rawFrame);
    std::string line;

    // First line is the command
    std::getline(iss, line);
    StompFrame frame(line);

    // Parse headers
    while (std::getline(iss, line) && !line.empty())
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            frame.addHeader(key, value);
        }
    }

    // The rest is the body
    std::ostringstream bodyStream;
    while (std::getline(iss, line))
    {
        bodyStream << line << "\n";
    }

    // Remove the trailing '\0'
    std::string body = bodyStream.str();
    if (!body.empty() && body.back() == '\0')
    {
        body.pop_back();
    }

    frame.setBody(body);
    return frame;
}
