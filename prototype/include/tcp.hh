#ifndef TCP_HH
#define TCP_HH

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <expected>
#include <string>

#include "data.hh"
#include "exception.hh"
#include "stream.hh"

EXCEPTION(SocketException);

template <typename Stream>
concept TcpStream = stream::RwStream<Stream, GenericData, error::SystemError>;

class TcpSocket {
public:
    template <typename Data = GenericData, typename Error = error::SystemError>
    constexpr auto write(const Data &data) -> std::expected<size_t, Error> {
        auto size = static_cast<size_t>(send(peer, data.data(), data.size(), 0));
        if (size < 0) {
            auto error = error::SystemError{std::errc{errno}};

            if (error.isSocketError()) {
                close(peer);
                peer = -1;
            }

            return std::unexpected{error};
        }
        return size;
    }

    template <typename Serialize = GenericData, size_t size = 1024, typename Return = Serialize, typename Error = error::SystemError,
              typename Result = std::expected<Serialize, Error>>
    constexpr auto read() -> Result {
        auto buffer = std::array<std::byte, size>{};
        if (recv(peer, buffer.data(), size, 0) < 0) {
            auto error = error::SystemError{std::errc{errno}};

            if (error.isSocketError()) {
                close(peer);
                peer = -1;
            }

            return std::unexpected{error};
        }
        return Serialize{buffer};
    }

    template <typename Serialize = GenericData, typename Return = Serialize, typename Error = error::SystemError,
              typename Result = std::expected<Serialize, Error>>
    constexpr auto read(size_t size) -> Result {
        auto buffer = std::vector<std::byte>(size);
        if (recv(peer, buffer.data(), size, 0) < 0) {
            auto error = error::SystemError{std::errc{errno}};

            if (error.isSocketError()) {
                close(peer);
                peer = -1;
            }

            return std::unexpected{error};
        }
        return buffer;
    }

    ~TcpSocket() {
        if (peer >= 0) {
            close(peer);
        }
    }

protected:
    int         peer = -1;
    sockaddr_in address;
};

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

        if (listen(sock, maxConcurrentClients)) {
            throw SocketException{"could not prepare for clients"};
        }

        if ((peer = accept(sock, nullptr, nullptr)) == -1) {
            throw SocketException{"could not accept client"};
        }
    }

private:
    static constexpr auto maxConcurrentClients = 1;

    uint16_t sock;
};

static_assert(TcpStream<TcpServer>);

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

static_assert(TcpStream<TcpClient>);

#endif
