#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class DaytimeClient {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    static const int DAYTIME_PORT = 13;
    static const int BUFFER_SIZE = 256;
    static const char* SERVER_IP;

public:
    DaytimeClient() : sockfd(-1) {
        memset(&server_addr, 0, sizeof(server_addr));
    }

    ~DaytimeClient() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }

    void initialize() {
        // Создание UDP сокета
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка создания сокета");
        }

        // Настройка адреса сервера
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(DAYTIME_PORT);
        if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Неверный IP адрес");
        }
    }

    std::string getTime() {
        // Отправка пустого датаграммы для запроса времени
        if (sendto(sockfd, "", 0, 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка отправки запроса");
        }

        std::cout << "Запрос отправлен на сервер " << SERVER_IP << ":" << DAYTIME_PORT << std::endl;

        // Получение ответа
        char buffer[BUFFER_SIZE];
        socklen_t addr_len = sizeof(server_addr);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr*)&server_addr, &addr_len);

        if (bytes_received < 0) {
            throw std::system_error(errno, std::system_category(), "Ошибка получения ответа");
        }

        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }
};

const char* DaytimeClient::SERVER_IP = "172.16.40.1";

int main() {
    try {
        DaytimeClient client;
        client.initialize();
        
        std::string time = client.getTime();
        std::cout << "Получено от сервера: " << time;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
