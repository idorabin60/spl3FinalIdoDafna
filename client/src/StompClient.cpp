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

	while (inputRunning)
	{
		std::cout << "Please enter a command:\n";
		std::getline(std::cin, userInput);

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
						std::cout << "Login successful.\n";
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
				std::cout << "Already logged in. Logout before logging in again.\n";
			}
		}
		else if (firstWord == "logout")
		{
			if (protocol.isLoggedIn())
			{
				receiptProcessed.store(false);

				// Send DISCONNECT frame
				std::string messageToBeSent = protocol.processCommand(userInput).serialize();
				connectionHandler->sendFrameAscii(messageToBeSent, '\0');
				// Stop receiver thread and clean up connection
				if (receiverThread.joinable())
				{
					receiverRunning = false;
					receiverThread.join();
				}

				if (connectionHandler != nullptr)
				{
					connectionHandler->close();
					delete connectionHandler;
					connectionHandler = nullptr;
				}

				protocol.reset();
				std::cout << "Logged out successfully. Program continues running...\n";
			}
			else
			{
				std::cout << "Not logged in. Nothing to log out.\n";
			}
		}

		else if (firstWord == "exit")
		{
			std::cout << "Exiting program...\n";
			inputRunning = false;

			// Stop receiver thread and clean up connection
			if (receiverThread.joinable())
			{
				receiverRunning = false;
				receiverThread.join();
			}

			if (connectionHandler != nullptr)
			{
				connectionHandler->close();
				delete connectionHandler;
				connectionHandler = nullptr;
			}
			break;
		}
		else if (firstWord == "join")
		{
			if (protocol.isLoggedIn())
			{
				std::string channel;
				inputStream >> channel;

				if (!channel.empty())
				{
					std::string messageToBeSent = protocol.processCommand(userInput).serialize();
					connectionHandler->sendFrameAscii(messageToBeSent, '\0');
					std::cout << "join successful " << channel << std::endl;
				}
				else
				{
					std::cout << "Error: 'join' command must specify a channel." << std::endl;
				}
			}
			else
			{
				std::cout << "You must be logged in to join a channel.\n";
			}
		}
		else if (firstWord == "report")
		{
			if (protocol.isLoggedIn())
			{
				std::string filePath;
				inputStream >> filePath;

				if (!filePath.empty())
				{
					std::vector<StompFrame> frames = protocol.processReportCommand(filePath);

					for (const StompFrame &frame : frames)
					{
						std::string serializedFrame = frame.serialize2();
						connectionHandler->sendFrameAscii(serializedFrame, '\0');
					}

					std::cout << "All events from file " << filePath << " have been sent." << std::endl;
				}
				else
				{
					std::cout << "Error: 'report' command must specify a file path.\n";
				}
			}
			else
			{
				std::cout << "You must be logged in to send a report.\n";
			}
		}
		else
		{
			std::cout << "Unknown command: " << firstWord << "\n";
		}
	}

	// Final cleanup
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

	std::cout << "Program terminated. Goodbye!\n";
	return 0;
}
