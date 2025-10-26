#include <array>
#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main ()
{
    const auto clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345);

    connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

    const std::string message = "Hello from client!";
    send(clientSocket, message.data(), message.size(), 0);

    close(clientSocket);
    return 0;
}
