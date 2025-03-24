#ifndef TCP_HH
#define TCP_HH

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <iterator>
#include <string>
#include <vector>

#include "data.hh"
#include "exception.hh"

EXCEPTION(SocketException);

class TcpSocket {
public:
    template <typename T>
    constexpr auto write(const T &data) {
        auto size = send(peer, data.data(), data.size(), 0);
        if (size < 0) {
            throw SocketException{"could not send message"};
        }
        return size;
    }

    constexpr auto read() -> Data {
        auto buffer = std::array<std::byte, max_message_size>{};
        auto size   = recv(peer, buffer.data(), buffer.size(), 0);

        auto out = std::vector<std::byte>{};
        out.reserve(size);
        std::ranges::copy(buffer, std::back_inserter(out));

        return Data{out};
    }

    ~TcpSocket() {
        if (peer >= 0) {
            close(peer);
        }
    }

protected:
    static constexpr auto max_message_size = 1024;
    int                   peer             = -1;
    sockaddr_in           address;
};

template <>
constexpr auto TcpSocket::write<Data>(const Data &data) {
    return write(data.raw);
}

class TcpServer : public TcpSocket {
public:
    constexpr TcpServer(std::string ipAddress, uint16_t port) {
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        address.sin_port        = htons(port);
        sock                    = socket(AF_INET, SOCK_STREAM, 0);

        if (sock < 0) {
            throw SocketException{"could not create socket"};
        }

        auto t = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(decltype(t)))) {
            throw SocketException{"could not set socket options"};
        }

        if (bind(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address))) {
            throw SocketException{"could not bind socket"};
        }

        if (listen(sock, max_concurrent_clients)) {
            throw SocketException{"could not prepare for clients"};
        }

        if ((peer = accept(sock, nullptr, nullptr)) == -1) {
            throw SocketException{"could not accept client"};
        }
    }

private:
    static constexpr auto max_concurrent_clients = 1;

    uint16_t sock;
};

class TcpClient : public TcpSocket {
public:
    constexpr TcpClient(std::string ipAddress, uint16_t port) {
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        address.sin_port        = htons(port);

        if ((peer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            throw SocketException{"could not create socket"};
        }

        if (connect(peer, reinterpret_cast<sockaddr *>(&address), sizeof(address))) {
            throw SocketException{"could not connect to server"};
        }
    }
};

#endif
