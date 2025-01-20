#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <sstream> // For std::istringstream
#include <regex>
#include "../include/StompFrame.h"
#include "../include/StompProtocol.h"
#include "../include/ConnectionHandler.h"
#include <mutex>
#include <condition_variable>

// Atomic flag to terminate the threads gracefully
std::atomic<bool> running(true);

// Global ConnectionHandler pointer
ConnectionHandler *connectionHandler = nullptr;

// Declare receiver thread globally
std::thread receiverThread;
std::mutex mtx;
std::condition_variable cv;
bool messageReceived = false; // Flag to indicate if a message is received

// Receiving Thread: Listens for messages from the server
void receivingThread(StompProtocol &protocol)
{
	while (running)
	{
		std::string serverMessage;

		// Wait for a message from the serve

		if (!connectionHandler->getLine(serverMessage))
		{
			std::cerr << "Error: Failed to receive data from the server." << std::endl;
			running = false; // Signal to terminate
			break;
		}
		// Parse and process the server response
		StompFrame frame = StompFrame::parse(serverMessage);
		std::cout << "Received from server: " << frame.serialize() << std::endl;
		protocol.processServerFrame(frame);
		std::unique_lock<std::mutex> lock(mtx);
		messageReceived = true;
		cv.notify_one(); // Notify the input thread
	}
	std::cout << "Receiving thread terminated." << std::endl;
}

// Input Thread: Handles user commands and sends them to the server
void inputThread()
{
	StompProtocol protocol;
	std::string userInput;
	bool isLoggedIn = false;

	while (running)
	{
		std::cout << "please enter a command\n";
		std::getline(std::cin, userInput); // Read the user's input

		if (userInput.empty())
		{
			std::cout << "No input provided. Please try again." << std::endl;
			continue;
		}

		std::istringstream inputStream(userInput); // Create a stream from the input
		std::string firstWord;
		inputStream >> firstWord; // Extract the first word

		if (firstWord == "login")
		{
			if (!isLoggedIn)
			{
				std::string hostPort, username, password;
				inputStream >> hostPort >> username >> password;

				// Regex to validate the format {host:port}
				std::regex hostPortRegex(R"(^([a-zA-Z0-9.-]+):(\d+)$)");
				std::smatch match;
				if (std::regex_match(hostPort, match, hostPortRegex) && !username.empty() && !password.empty())
				{
					std::string host = match[1];	  // Extract host
					std::string port = match[2];	  // Extract port
					int portNumber = std::stoi(port); // Convert port string to integer

					// Create a new ConnectionHandler
					connectionHandler = new ConnectionHandler(host, portNumber);
					if (connectionHandler->connect())
					{
						std::cout << "Connected successfully to " << host << " on port " << portNumber << std::endl;

						// Start the Receiving Thread
						if (receiverThread.joinable())
						{
							receiverThread.join(); // Wait for any existing thread to finish
						}
						receiverThread = std::thread(receivingThread, std::ref(protocol));
						isLoggedIn = true;

						// Send the login command
						std::string messageToBeSent = protocol.processCommand(userInput).serialize();
						std::cout << messageToBeSent;
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
					std::cout << "login command needs 3 args: {host:port} {username} {password}" << std::endl;
				}
			}
			else
			{
				std::cout << "user already logedin";
			}
		}
		else if (firstWord == "exit")
		{
			std::cout << "Exiting program..." << std::endl;
			running = false; // Signal to terminate
			if (receiverThread.joinable())
			{
				receiverThread.join();
			}
			break;
		}
		else
		{
			std::cout << "Unknown command: " << firstWord << std::endl;
		}
		std::cout << "dafna cute";
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, []
				{ return messageReceived || !running; });
	}
}

int main(int argc, char *argv[])
{
	// Start the Input Thread
	std::thread inputThreadWorker(inputThread);

	// Wait for the Input Thread to finish
	inputThreadWorker.join();

	// Ensure the Receiving Thread is joined
	if (receiverThread.joinable())
	{
		receiverThread.join();
	}

	std::cout << "Program terminated. Goodbye!" << std::endl;
	return 0;
}
