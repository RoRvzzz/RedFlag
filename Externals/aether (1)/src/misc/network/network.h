#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace network {

    struct socket_wrapper {
        SOCKET sock = INVALID_SOCKET;

        socket_wrapper() = default;

        explicit socket_wrapper(SOCKET s) : sock(s) {}

        ~socket_wrapper() {
            if (sock != INVALID_SOCKET)
                closesocket(sock);
        }

        socket_wrapper(const socket_wrapper&) = delete;
        socket_wrapper& operator=(const socket_wrapper&) = delete;

        socket_wrapper(socket_wrapper&& other) noexcept : sock(other.sock) {
            other.sock = INVALID_SOCKET;
        }

        socket_wrapper& operator=(socket_wrapper&& other) noexcept {
            if (this != &other) {
                if (sock != INVALID_SOCKET)
                    closesocket(sock);
                sock = other.sock;
                other.sock = INVALID_SOCKET;
            }
            return *this;
        }

        bool valid() const { return sock != INVALID_SOCKET; }
    };

    struct winsock_init {
        winsock_init() {
            WSADATA wsa_data;
            WSAStartup(MAKEWORD(2, 2), &wsa_data);
        }

        ~winsock_init() {
            WSACleanup();
        }
    };

    inline bool initialize() {
        static winsock_init wsi;
        return true;
    }

    inline socket_wrapper create_tcp_socket() {
        initialize();
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        return socket_wrapper(s);
    }

    inline bool connect(socket_wrapper& sock, const std::string& ip, unsigned short port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        int result = ::connect(sock.sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        return result != SOCKET_ERROR;
    }

    inline bool bind_listen(socket_wrapper& sock, unsigned short port, int backlog = SOMAXCONN) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock.sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
            return false;

        if (listen(sock.sock, backlog) == SOCKET_ERROR)
            return false;

        return true;
    }

    inline socket_wrapper accept_connection(const socket_wrapper& sock) {
        sockaddr_in addr{};
        int addrlen = sizeof(addr);
        SOCKET client = accept(sock.sock, reinterpret_cast<sockaddr*>(&addr), &addrlen);
        return socket_wrapper(client);
    }

    inline int send_data(const socket_wrapper& sock, const char* data, int length) {
        return send(sock.sock, data, length, 0);
    }

    inline int recv_data(const socket_wrapper& sock, char* buffer, int length) {
        return recv(sock.sock, buffer, length, 0);
    }

    inline bool send_all(const socket_wrapper& sock, const char* data, int length) {
        int total_sent = 0;
        while (total_sent < length) {
            int sent = send(sock.sock, data + total_sent, length - total_sent, 0);
            if (sent == SOCKET_ERROR || sent == 0) return false;
            total_sent += sent;
        }
        return true;
    }

    inline bool recv_all(const socket_wrapper& sock, char* buffer, int length) {
        int total_recv = 0;
        while (total_recv < length) {
            int recvd = recv(sock.sock, buffer + total_recv, length - total_recv, 0);
            if (recvd == SOCKET_ERROR || recvd == 0) return false;
            total_recv += recvd;
        }
        return true;
    }

}
