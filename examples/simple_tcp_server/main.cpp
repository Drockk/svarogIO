#include <array>
#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main ()
{
    const auto serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345);

    bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

    listen(serverSocket, 5);

    const auto clientSocket = accept(serverSocket, nullptr, nullptr);

    std::array<std::byte, 1024> buffer{};

    recv(clientSocket, buffer.data(), buffer.size(), 0);

    std::cout << "Received data from client: " << std::string(reinterpret_cast<char*>(buffer.data()), buffer.size()) << std::endl;

    close(serverSocket);
    return 0;
}
