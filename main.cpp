#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <unordered_map>
#include <winsock2.h>
#include <thread> 
#include <random>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

SOCKET clientSocket; 
bool shouldExit = false;

void cleanup() {
    closesocket(clientSocket);
    WSACleanup();
}

std::unordered_map<std::string, std::string> parse_params(const std::string& url) {
    std::unordered_map<std::string, std::string> params;
    size_t params_start = url.find("?");

    if (params_start != std::string::npos) {
        std::string params_string = url.substr(params_start + 1);
        size_t pos = 0;

        while ((pos = params_string.find("&")) != std::string::npos) {
            std::string param = params_string.substr(0, pos);
            size_t equals_pos = param.find("=");
            if (equals_pos != std::string::npos) {
                std::string key = param.substr(0, equals_pos);
                std::string value = param.substr(equals_pos + 1);
                params[key] = value;
            }
            params_string.erase(0, pos + 1);
        }

        size_t equals_pos = params_string.find("=");
        if (equals_pos != std::string::npos) {
            std::string key = params_string.substr(0, equals_pos);
            std::string value = params_string.substr(equals_pos + 1);
            params[key] = value;
        }
    }

    return params;
}

std::string sendMessage(const std::string& message) {
    std::string response;

    if (send(clientSocket, message.c_str(), message.size(), 0) != SOCKET_ERROR) {
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response = buffer;
        }
        else if (bytesRead == 0) {
            response = "Connection closed by server.";
        }
        else {
            response = "Error while receiving response.";
        }
    }
    else {
        response = "Failed to send message.";
    }

    return response;
}
std::string buildpacket(const std::string& messageId,
    const std::string& topic,
    const std::string& data,
    const std::string& dataType) {
    std::stringstream ss;
    ss << R"({
        ")" << messageId << R"(": {
            "messageid": ")" << messageId << R"(",
            "topic": ")" << topic << R"(",
            "data": ")" << data << R"(",
            "datatype": ")" << dataType << R"("
        }
    })";

    return ss.str();
}
std::string generateUUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);


    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << part1;


    return ss.str();
}

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock." << std::endl;
        return 1;
    }

    if (argc > 1) {
        std::string argStr = argv[1];
        std::unordered_map<std::string, std::string> params = parse_params(argStr);

        autop peram1It = params.find("peram1");
        auto peram2It = params.find("peram2");
        auto peram3It = params.find("peram3");
        auto peram4It = params.find("peram4");

        if (peram1It != params.end() && peram2It != params.end() && peram3It != params.end() && peram4It != params.end()) {
            std::string peram1 = peram1It->second;
            uint64_t peram2 = std::stoull(peram2It->second);
            uint64_t peram3 = std::stoull(peram3It->second);
            std::string peram4 = peram4It->second;

            if (peram1 == peram1) {
                std::cout << "Authenticated!" << std::endl;
                clientSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "Failed to create socket." << std::endl;
                    WSACleanup();
                    return 1;
                }

                sockaddr_in serverAddress;
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(8080);
                serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

                if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
                    std::cerr << "Failed to connect to the server." << std::endl;
                    cleanup();
                    return 1;
                }
                //basic traffic test
                std::string uuid = generateUUID();
                std::string testingpacket = buildpacket(uuid, "message", "ping", "json");
                std::string verifypacket = buildpacket(uuid, "verify", peram1+"=>"+std::to_string(peram3), "json");
                std::string response = sendMessage(testingpacket);
                if (response != "pong") {
                    std::cout << "Sever Is too Busy";
                    cleanup();
                }
                std::string verifycallback = sendMessage(verifypacket);
                
                if (verifycallback != "ccns") {
                    std::cout << "Sever Is too Busy";
                    cleanup();
                }
                


            }
            else {
                std::cout << "Authentication failed" << std::endl;
            }
        }
        else {
            std::cout << "Missing parameter." << std::endl;
        }
    }
    else {
        std::cout << "No parameters provided." << std::endl;
    }

    atexit(cleanup); 
    return 0;
}
