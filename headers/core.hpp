#ifndef WEBSOCKSERVER_CORE_HPP
#define WEBSOCKSERVER_CORE_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h> // Windows Sockets 2 header
#include <ws2tcpip.h> // Windows Sockets 2 TCP/IP header
#include <mswsock.h> // Microsoft Windows Sockets header
#include <sqlite3.h> // SQLite header

#include <vector>
#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <bitset>
#include <random>
#include <nlohmann/json.hpp>
#include <fstream>
#include <queue>

#define INIT_BUF_MEM 1024 // Initial Buffer memory

#define WEB_SOCK_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" // Magic-String, explained in docs

#endif //WEBSOCKSERVER_CORE_HPP
