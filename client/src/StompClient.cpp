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

// Atomic flag to terminate the threads gracefully
std::atomic<bool> running(true);

// Global ConnectionHandler pointer
ConnectionHandler *connectionHandler = nullptr;

// Declare receiver thread globally
std::thread receiverThread;
std::mutex mtx;
std::condition_variable cv;
bool messageReceived = false;			   // Flag to indicate if a message is received
std::atomic<bool> receiptProcessed(false); // To track receipt handling

// Receiving Thread: Listens for messages from the server
void receivingThread(StompProtocol &protocol)
{
	while (running)
	{
		std::string serverMessage;

		if (!connectionHandler->getLine(serverMessage))
		{
			std::cout << "ido look here";
			running = false; // Signal to terminate
			break;
		}

		// Parse and process the server response
		StompFrame frame = StompFrame::parse(serverMessage);
		if (frame.getCommand() == "RECEIPT")
		{
			receiptProcessed = true; // Indicate receipt was processed
		}
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
						receiverThread = std::thread(receivingThread, std::ref(protocol));
						isLoggedIn = true;

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
					std::cout << "login command needs 3 args: {host:port} {username} {password}" << std::endl;
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
			running = false;
			if (receiverThread.joinable())
			{
				receiverThread.join();
			}
			break;
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
				connectionHandler->sendFrameAscii(messageToBeSent, '\0');

				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, []
						{ return messageReceived || !running; });
				messageReceived = false; // Reset the flag

				receiptProcessed = false; // Reset after handling

				std::cout << "join successful " << channel << std::endl;
			}
			else
			{
				std::cout << "Error: 'join' command must have exactly two words." << std::endl;
			}
		}
	else if (firstWord == "report") {
    std::string filePath;
    inputStream >> filePath;

    // Process the file and get the list of frames
    std::vector<StompFrame> frames = protocol.processReportCommand(filePath);

    // Send each frame
    for (const StompFrame &frame : frames) {
        std::string serializedFrame = frame.serialize();
        std::cout << "Sending frame:\n" << serializedFrame << std::endl;

        connectionHandler->sendFrameAscii(serializedFrame, '\0');

        // Wait for acknowledgment (if needed)
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []
                { return messageReceived || !running; });
        messageReceived = false; // Reset the flag

        receiptProcessed = false; // Reset after handling
    }

    std::cout << "All events from file " << filePath << " have been sent." << std::endl;
}


		else
		{
			std::cout << "Unknown command: " << firstWord << std::endl;
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
