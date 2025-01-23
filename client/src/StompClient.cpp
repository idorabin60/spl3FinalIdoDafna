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
void receivingThread(StompProtocol &protocol)
{
	while (receiverRunning)
	{
		std::string serverMessage;
		bool sucsess = connectionHandler->getLine(serverMessage);
		if (!sucsess)
		{
			std::cout<<serverMessage<<std::endl;
			protocol.processServerFrame(serverMessage);
			std::cout << "An error has been recived logging out from the system";
			protocol.reset();
			delete connectionHandler;
			connectionHandler = nullptr;
			break;
		}
		else
		{
			protocol.processServerFrame(serverMessage);
		}
	}//bla
}

// Input Thread: Handles user commands and sends them to the server
void inputThread()
{
	StompProtocol protocol;
	std::string userInput;

	while (inputRunning)
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
					std::string port = match[2];
					int portNumber = std::stoi(port);

					connectionHandler = new ConnectionHandler(host, portNumber);
					if (connectionHandler->connect())
					{
						if (receiverThread.joinable())
						{
							receiverThread.join();
						}
						receiverRunning = true; // Restart receiverRunning
						receiverThread = std::thread(receivingThread, std::ref(protocol));
						protocol.setLoggedIn(true);
						std::cout << "STRATING NEW THREAD!!!!!!!!!!!!!!!!!!!!!";

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
			std::istringstream wordStream(userInput);
			std::string command, channel;
			wordStream >> command >> channel;
			if (!command.empty() && !channel.empty() && wordStream.eof())
			{ // Check if there are exactly two words
				std::string messageToBeSent = protocol.processCommand(userInput).serialize();
				std::cout << messageToBeSent;
				connectionHandler->sendFrameAscii(messageToBeSent, '\0');
				std::cout << "Unsubscribe :" << channel << std::endl;
			}
			else
			{
				std::cout << "Error: 'Exit' command must have exactly two words." << std::endl;
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

				// Wait for receipt confirmation
				{
					std::unique_lock<std::mutex> lock(mtx);
					std::cout << "Waiting for receipt confirmation..." << std::endl;

					cv.wait(lock, []
							{ return receiptProcessed.load(); });
				}

				std::cout << "Receipt confirmed. Proceeding to logout..." << std::endl;

				// Stop receiver thread
				std::cout << "Setting receiverRunning to false..." << std::endl;
				std::cout << "Attempting to join receiver thread..." << std::endl;

				receiverRunning = false;
				std::cout << "COOOL" << std::endl;

				receiverThread.detach(); // LAST RESORT IDO!!!
				std::cout << "Receiver thread stopped." << std::endl;

				// Reset protocol state
				protocol.reset();

				// Delete connection handler
				delete connectionHandler;
				connectionHandler = nullptr;

				std::cout << "Logged out successfully." << std::endl;
			}
			else
			{
				std::cout << "Not logged in." << std::endl;
			}
		}
		else if (firstWord == "join")
		{
			std::istringstream wordStream(userInput);
			std::string command, channel;

			// Extract the first and second words
			wordStream >> command >> channel;

			// Ensure exactly two words exist
			if (!command.empty() && !channel.empty() && wordStream.eof()) // Check if there are exactly two words
			{
				std::string messageToBeSent = protocol.processCommand(userInput).serialize();
				std::cout << messageToBeSent;
				connectionHandler->sendFrameAscii(messageToBeSent, '\0');
				std::cout << "joind :" << channel << std::endl;
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

			// Process the file and get the list of frames
			std::vector<StompFrame> frames = protocol.processReportCommand(filePath);

			// Send each frame
			for (const StompFrame &frame : frames)
			{
				std::string serializedFrame = frame.serialize2();
				std::cout << "Sending frame:\n"
						  << serializedFrame << std::endl;

				connectionHandler->sendFrameAscii(serializedFrame, '\0');

				// Wait for acknowledgment (if needed)
			}

			std::cout << "All events from file " << filePath << " have been sent." << std::endl;
		}
	}
}

int main(int argc, char *argv[])
{
	std::thread inputThreadWorker(inputThread);
	inputThreadWorker.join();

	if (receiverThread.joinable())
	{
		receiverThread.join();
	}

	std::cout << "Program terminated. Goodbye!" << std::endl;
	return 0;
}
