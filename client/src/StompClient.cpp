#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <sstream>
#include <regex>
#include <mutex>
#include <condition_variable>
#include "../include/StompFrame.h"
#include "../include/StompProtocol.h"
#include "../include/ConnectionHandler.h"

// Atomic flags for controlling thread lifetimes
std::atomic<bool> inputRunning(true);
std::atomic<bool> receiverRunning(true);

// Global ConnectionHandler pointer
ConnectionHandler *connectionHandler = nullptr;

// Declare receiver thread globally
std::thread receiverThread;

// Global synchronization primitives
std::condition_variable cv;
std::mutex mtx;
std::atomic<bool> receiptProcessed(false);

// Receiving Thread: Listens for messages from the server
void runReceiver(StompProtocol &protocol)
{
    while (receiverRunning)
    {
        std::string serverMessage;
        bool success = connectionHandler->getLine(serverMessage);
        if (!success)
        {
            std::cout << "Disconnected from server.\n";
            break;
        }
        protocol.processServerFrame(serverMessage);
    }
    std::cout << "Receiver thread finished.\n";
}


int main(int argc, char *argv[])
{
    StompProtocol protocol;
    std::string userInput;

    while (receiverRunning)
    {
        std::cout << "Please enter a command:\n";
        std::getline(std::cin, userInput); // Read the user's input

        if (userInput.empty())
        {
            std::cout << "No input provided. Please try again." << std::endl;
            continue;
        }

        std::istringstream inputStream(userInput);
        std::string firstWord;
        inputStream >> firstWord;

        if (firstWord == "login")
        {
            if (!protocol.isLoggedIn())
            {
                std::string hostPort, username, password;
                inputStream >> hostPort >> username >> password;

                std::regex hostPortRegex(R"(^([a-zA-Z0-9.-]+):(\d+)$)");
                std::smatch match;
                if (std::regex_match(hostPort, match, hostPortRegex) && !username.empty() && !password.empty())
                {
                    std::string host = match[1];
                    int portNumber = std::stoi(match[2]);

                    connectionHandler = new ConnectionHandler(host, portNumber);
                    if (connectionHandler->connect())
                    {
                        receiverRunning = true;
                        receiverThread = std::thread(runReceiver, std::ref(protocol));
                        protocol.setLoggedIn(true);
                        std::string messageToBeSent = protocol.processCommand(userInput).serialize();
                        connectionHandler->sendFrameAscii(messageToBeSent, '\0');
                    }
                    else
                    {
                        std::cerr << "Failed to connect to " << host << " on port " << portNumber << std::endl;
                        delete connectionHandler;
                        connectionHandler = nullptr;
                    }
                }
                else
                {
                    std::cout << "Login command needs 3 args: {host:port} {username} {password}" << std::endl;
                }
            }
            else
            {
                std::cout << "User already logged in." << std::endl;
            }
        }
        else if (firstWord == "exit")
        {
            std::cout << "Exiting program..." << std::endl;
            inputRunning = false;
            receiverRunning = false;

            if (receiverThread.joinable())
            {
                receiverThread.join();
            }
            break;
        }
        else if (firstWord == "logout")
{
    if (protocol.isLoggedIn())
    {
        receiptProcessed.store(false);

        // Send DISCONNECT frame
        std::string messageToBeSent = protocol.processCommand(userInput).serialize();
		std::cout<<messageToBeSent;
        connectionHandler->sendFrameAscii(messageToBeSent, '\0');

        // Wait for receipt confirmation
        {
            std::unique_lock<std::mutex> lock(mtx);
            std::cout << "Waiting for receipt confirmation..." << std::endl;
            cv.wait(lock, [] { return receiptProcessed.load(); });
        }

        std::cout << "Receipt confirmed. Proceeding to logout..." << std::endl;

        // Close the connection for this client
        if (connectionHandler != nullptr)
        {
            connectionHandler->close();
            delete connectionHandler;
            connectionHandler = nullptr;
        }

        // Reset protocol state for the client
        protocol.reset();

        std::cout << "Logged out successfully. Program continues running..." << std::endl;
    }
    else
    {
        std::cout << "Not logged in. Nothing to log out." << std::endl;
    }
}

        else if (firstWord == "join")
        {
            std::istringstream wordStream(userInput);
            std::string command, channel;

            wordStream >> command >> channel;

            if (!command.empty() && !channel.empty() && wordStream.eof())
            {
                std::string messageToBeSent = protocol.processCommand(userInput).serialize();
                connectionHandler->sendFrameAscii(messageToBeSent, '\0');
                std::cout << "join successful " << channel << std::endl;
            }
            else
            {
                std::cout << "Error: 'join' command must have exactly two words." << std::endl;
            }
        }
        else if (firstWord == "report")
        {
            std::string filePath;
            inputStream >> filePath;

            std::vector<StompFrame> frames = protocol.processReportCommand(filePath);

            for (const StompFrame &frame : frames)
            {
                std::string serializedFrame = frame.serialize2();
                connectionHandler->sendFrameAscii(serializedFrame, '\0');
            }

            std::cout << "All events from file " << filePath << " have been sent." << std::endl;
        }
        else if (firstWord == "summarize")
        {
            std::string channel, user, file;
            inputStream >> channel >> user >> file;

            if (!channel.empty() && !user.empty() && !file.empty())
            {
                protocol.summarize(channel, user, file);
            }
            else
            {
                std::cout << "Error: 'summarize' command requires {channel} {user} {file} arguments." << std::endl;
            }
        }
    }

    if (receiverThread.joinable())
    {
        receiverThread.join();
    }

    if (connectionHandler != nullptr)
    {
        connectionHandler->close();
        delete connectionHandler;
        connectionHandler = nullptr;
    }

    protocol.reset();

    std::cout << "Program terminated. Goodbye!" << std::endl;
    return 0;
}
