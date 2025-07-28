#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>

class TCPServer {
private:
    int server_fd;
    struct sockaddr_in address;
    static const size_t BUFFER_SIZE = 4096;
    static const int MAX_CLIENTS = 10;

public:
    TCPServer() : server_fd(-1) {}

    ~TCPServer() {
        cleanup();
    }

    int initialize(const std::string& port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            std::cerr << "Socket creation failed\n";
            return -1;
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            std::cerr << "Setsockopt failed\n";
            return -1;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(std::stoi(port));

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed\n";
            return -1;
        }

        if (listen(server_fd, MAX_CLIENTS) < 0) {
            std::cerr << "Listen failed\n";
            return -1;
        }

        std::cout << "TCP server listening on port " << port << std::endl;
        return 0;
    }

    int waitForConnection() {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0) {
            std::cerr << "Accept failed\n";
            return -1;
        }

        std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        handleClient(client_socket);
        close(client_socket);
        return 0;
    }

    void handleClient(int client_socket) {
        char buffer[BUFFER_SIZE];
        
        // Receive message from client
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Message received: " << buffer << std::endl;
            
            // Send response back to client
            std::string response = "Hello from TCP server!";
            send(client_socket, response.c_str(), response.length(), 0);
            std::cout << "Response sent: " << response << std::endl;
        } else {
            std::cerr << "Failed to receive message\n";
        }
    }

    int sendMessage(int client_socket, const std::string& message) {
        ssize_t bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Failed to send message\n";
            return -1;
        }
        std::cout << "Message sent: " << message << std::endl;
        return 0;
    }

    int receiveMessage(int client_socket) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Message received: " << buffer << std::endl;
            return 0;
        } else if (bytes_received == 0) {
            std::cout << "Client disconnected\n";
            return 1;
        } else {
            std::cerr << "Failed to receive message\n";
            return -1;
        }
    }

private:
    void cleanup() {
        if (server_fd >= 0) {
            close(server_fd);
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    TCPServer server;
    
    int ret = server.initialize(argv[1]);
    if (ret) {
        std::cerr << "Failed to initialize server\n";
        return ret;
    }

    while (true) {
        ret = server.waitForConnection();
        if (ret) {
            std::cerr << "Connection handling failed\n";
            break;
        }
    }

    return 0;
}