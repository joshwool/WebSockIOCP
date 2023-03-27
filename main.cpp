#include <websockiocp.hpp>
#include <core.hpp>

int main() {

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) == 0) {
        Server server("80", "127.0.0.1", "c:/database/typeprac.db", 20, 20);

		std::cout << "Server Initialised" << std::endl;

        server.Setup();

        server.Run();
    }
    else {
        std::cout << "WSAStartup() failed: " << WSAGetLastError() << std::endl;
    }
    return 0;
}