#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class TCPClient {
private:
    int sock_fd;
    struct sockaddr_in server_addr;
    static const size_t BUFFER_SIZE = 4096;

public:
    TCPClient() : sock_fd(-1) {}

    ~TCPClient() {
        cleanup();
    }

    int initialize() {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            std::cerr << "Socket creation failed\n";
            return -1;
        }
        return 0;
    }

    int connectToServer(const std::string& server_ip, const std::string& port) {
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(std::stoi(port));

        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address or address not supported\n";
            return -1;
        }

        if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed\n";
            return -1;
        }

        std::cout << "Connected to TCP server at " << server_ip << ":" << port << std::endl;
        return 0;
    }

    int sendMessage(const std::string& message) {
        if (sock_fd < 0) {
            std::cerr << "Not connected to server\n";
            return -1;
        }

        ssize_t bytes_sent = send(sock_fd, message.c_str(), message.length(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Failed to send message\n";
            return -1;
        }

        std::cout << "Message sent: " << message << std::endl;
        return 0;
    }

    int receiveMessage() {
        if (sock_fd < 0) {
            std::cerr << "Not connected to server\n";
            return -1;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        ssize_t bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Message received: " << buffer << std::endl;
            return 0;
        } else if (bytes_received == 0) {
            std::cout << "Server disconnected\n";
            return 1;
        } else {
            std::cerr << "Failed to receive message\n";
            return -1;
        }
    }

    int performHandshake() {
        // Send initial message
        int ret = sendMessage("Hello from TCP client!");
        if (ret) {
            return ret;
        }

        // Receive response
        ret = receiveMessage();
        return ret;
    }

private:
    void cleanup() {
        if (sock_fd >= 0) {
            close(sock_fd);
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    TCPClient client;
    
    int ret = client.initialize();
    if (ret) {
        std::cerr << "Failed to initialize client\n";
        return ret;
    }

    ret = client.connectToServer(argv[1], argv[2]);
    if (ret) {
        std::cerr << "Failed to connect to server\n";
        return ret;
    }

    // Perform message exchange
    ret = client.performHandshake();
    if (ret) {
        std::cerr << "Message exchange failed\n";
        return ret;
    }

    return 0;
}