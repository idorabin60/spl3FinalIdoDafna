#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include "ConnectionHandler.h"
#include "StompProtocol.h"

// פונקציה שרצה ב-Thread נפרד:
// קוראת Frames מהשרת בלולאה, עד שנדרש להפסיק.
void serverReader(ConnectionHandler* connectionHandler, 
                  StompProtocol* protocol, 
                  std::atomic_bool* stopReading) 
{
    while (!(*stopReading)) {
        std::string answer;
        bool success = connectionHandler->getLine(answer); 
        if (!success) {
            std::cerr << "Disconnected or error reading from server.\n";
            // אפשר לסמן הפסקה
            break;
        }
        // אצלכם אולי צריך להוריד \n או לטפל ב-\0
        // כאן מעבירים את התשובה לפרוטוקול
        protocol->processServerFrame(answer);

        // ייתכן delay קל שימנע 100% CPU
        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cerr << "serverReader thread finished.\n";
}

int main(int argc, char** argv) {
    StompProtocol protocol;       // היגיון STOMP בצד הלקוח
    ConnectionHandler* ch = nullptr; 
    std::thread serverThread;     // thread לקריאת Frames מהשרת
    std::atomic_bool stopReading(false);
    bool isLoggedIn = false;

    // לולאת פקודות מהמשתמש
    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF / error
            break;
        }
        // מפצלים את השורה למילים
        std::vector<std::string> tokens = protocol.split(line, ' ');
        if (tokens.empty()) {
            continue;
        }
        std::string cmd = tokens[0];

        if (cmd == "login") {
            if (isLoggedIn) {
                std::cout << "Already logged in. Please logout first.\n";
                continue;
            }
            // נניח פורמט: login {host:port} {user} {pass}
            if (tokens.size() < 4) {
                std::cout << "Usage: login {host:port} {username} {password}\n";
                continue;
            }
            // מפענחים host + port
            std::string hostport = tokens[1];
            std::string host = hostport.substr(0, hostport.find(':'));
            short port = std::stoi(hostport.substr(hostport.find(':') + 1));
            std::string user = tokens[2];
            std::string pass = tokens[3];

            // יוצרים ConnectionHandler חדש
            if (ch != nullptr) {
                delete ch;
                ch = nullptr;
            }
            ch = new ConnectionHandler(host, port);

            // מתחברים
            if (!ch->connect()) {
                std::cout << "Could not connect to server\n";
                delete ch;
                ch = nullptr;
                continue;
            }
            // שולחים Frame CONNECT
            std::string connectFrame = protocol.buildConnectFrame(hostport, user, pass);
            ch->sendFrameAscii(connectFrame, '\0');

            // כעת אנו מחליטים להרים Thread לקריאת השרת
            stopReading = false;
            serverThread = std::thread(serverReader, ch, &protocol, &stopReading);

            isLoggedIn = true;
            std::cout << "Login command processed. (CONNECT frame sent)\n";
        }
        else if (cmd == "logout") {
            if (!isLoggedIn) {
                std::cout << "Not logged in.\n";
                continue;
            }
            // שולחים DISCONNECT
            int receiptId = protocol.getNextReceiptId(); // נניח יש פונקציה כזאת
            std::string disconnectFrame = protocol.buildDisconnectFrame(receiptId);
            ch->sendFrameAscii(disconnectFrame, '\0');

            // נניח שתחנתם בפרוטוקול שאתם מחכים ל-RECEIPT של DISCONNECT
            // אפשר להמתין זמן מסוים או לבדוק שהתקבל...
            // בינתיים אפשר לעצור את לולאת הקריאה:
            stopReading = true; 
            if (serverThread.joinable()) {
                serverThread.join();
            }
            ch->close();
            delete ch;
            ch = nullptr;

            isLoggedIn = false;
            std::cout << "Logged out.\n";
        }
        else if (cmd == "join") {
            if (!isLoggedIn) {
                std::cout << "You must login first.\n";
                continue;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: join {channel}\n";
                continue;
            }
            std::string channel = tokens[1];
            // בונים SUBSCRIBE Frame
            int subId = protocol.getNextSubId();
            int receiptId = protocol.getNextReceiptId();
            std::string subFrame = protocol.buildSubscribeFrame(channel, subId, receiptId);
            ch->sendFrameAscii(subFrame, '\0');
            std::cout << "join command processed.\n";
        }
        else if (cmd == "report") {
            // וכו’...
            // אפשר לקרוא קובץ JSON וכו’
            // שולחים SEND frames אחד אחד
        }
        else {
            // כל פקודה אחרת
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }

    // אם יוצאים מה-while, סוגרים הכל
    stopReading = true;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    if (ch != nullptr) {
        ch->close();
        delete ch;
        ch = nullptr;
    }

    std::cout << "Client finished.\n";
    return 0;
}