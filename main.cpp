#include <websockiocp.hpp>

int main() {
    Server server = Server(
            "80",
            500,
            20,
            500);
}