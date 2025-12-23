#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class EchoClient {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    static const int ECHO_PORT = 7;
    static const int BUFFER_SIZE = 1024;
    static const char* SERVER_IP;

public:
    EchoClient() : sockfd(-1) {
        memset(&server_addr, 0, sizeof(server_addr));
    }

    ~EchoClient() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }

    void initialize() {
        // Создание TCP сокета
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка создания сокета");
        }

        // Настройка адреса сервера
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(ECHO_PORT);
        if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Неверный IP адрес");
        }
    }

    std::string sendMessage(const std::string& message) {
        // Подключение к серверу
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка подключения к серверу");
        }

        std::cout << "Подключено к серверу " << SERVER_IP << ":" << ECHO_PORT << std::endl;

        // Отправка сообщения
        int bytes_sent = send(sockfd, message.c_str(), message.length(), 0);
        if (bytes_sent < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка отправки сообщения");
        }

        std::cout << "Отправлено: " << message << std::endl;

        // Получение ответа
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка получения ответа");
        }

        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }
};

const char* EchoClient::SERVER_IP = "172.16.40.1";

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <сообщение>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        EchoClient client;
        client.initialize();
        
        std::string response = client.sendMessage(argv[1]);
        std::cout << "Получено: " << response << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
