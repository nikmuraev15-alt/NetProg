#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

const int DEFAULT_PORT = 13;
const std::string DEFAULT_SERVER = "172.16.40.1";
const int BUFFER_SIZE = 1024;

void show_help() {
    std::cout << "Клиент службы Daytime (UDP)\n";
    std::cout << "Использование: daytime_client [-s сервер] [-p порт] [-h] [--test]\n";
    std::cout << "Параметры:\n";
    std::cout << "  -s, --server АДРЕС   IP-адрес или доменное имя сервера (по умолчанию: 172.16.40.1)\n";
    std::cout << "  -p, --port ПОРТ      Номер порта (по умолчанию: 13)\n";
    std::cout << "  -h, --help          Показать эту справку\n";
    std::cout << "  --test              Использовать тестовый сервер (localhost:1313)\n";
    std::cout << "Примеры:\n";
    std::cout << "  daytime_client                    # Сервер кафедры\n";
    std::cout << "  daytime_client --test             # Локальный тестовый сервер\n";
    std::cout << "  daytime_client -s localhost -p 1313  # Ручное указание тестового сервера\n";
    std::cout << "  daytime_client -s time.google.com    # Публичный сервер\n";
}

bool resolve_hostname(const std::string& hostname, std::string& ip_address) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    
    int status = getaddrinfo(hostname.c_str(), NULL, &hints, &result);
    if (status != 0) {
        std::cerr << "Ошибка разрешения доменного имени: " << gai_strerror(status) << "\n";
        return false;
    }
    
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN);
    
    ip_address = ip_str;
    freeaddrinfo(result);
    return true;
}

bool connect_to_daytime_server(const std::string& server, int port) {
    // Создание UDP сокета
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return false;
    }
    
    // Установка таймаута 5 секунд
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Настройка адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    std::string resolved_server = server;
    
    // Если это не IP-адрес, пытаемся разрешить доменное имя
    if (inet_pton(AF_INET, server.c_str(), &server_addr.sin_addr) <= 0) {
        std::cout << "Разрешение доменного имени: " << server << "...\n";
        if (!resolve_hostname(server, resolved_server)) {
            close(sockfd);
            return false;
        }
        std::cout << "Решенный IP-адрес: " << resolved_server << "\n";
        
        // Повторная попытка с разрешенным IP
        if (inet_pton(AF_INET, resolved_server.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Ошибка преобразования адреса: " << resolved_server << "\n";
            close(sockfd);
            return false;
        }
    }
    
    std::cout << "Подключение к " << server << ":" << port << " (" << resolved_server << ")...\n";
    
    // Отправка пустого datagram для получения времени
    if (sendto(sockfd, nullptr, 0, 0, 
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка отправки запроса\n";
        close(sockfd);
        return false;
    }
    
    // Получение ответа
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                               (struct sockaddr*)&server_addr, &addr_len);
    
    if (recv_len < 0) {
        std::cerr << "Ошибка получения ответа (таймаут или недоступность сервера)\n";
        close(sockfd);
        return false;
    }
    
    buffer[recv_len] = '\0';
    std::cout << "Текущее время:\n";
    std::cout << buffer;
    
    close(sockfd);
    return true;
}

int main(int argc, char* argv[]) {
    std::string server = DEFAULT_SERVER;
    int port = DEFAULT_PORT;
    bool test_mode = false;
    
    // Разбор аргументов командной строки
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "--test") {
            test_mode = true;
            server = "localhost";
            port = 1313;
        } else if (arg == "-s" || arg == "--server") {
            if (i + 1 < argc) {
                server = argv[++i];
            } else {
                std::cerr << "Ошибка: не указан адрес сервера\n";
                return 1;
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = std::atoi(argv[++i]);
            } else {
                std::cerr << "Ошибка: не указан порт\n";
                return 1;
            }
        }
    }
    
    if (test_mode) {
        std::cout << "=== ТЕСТОВЫЙ РЕЖИМ ===\n";
    }
    
    // Попытка подключения к серверу
    if (!connect_to_daytime_server(server, port)) {
        if (!test_mode && server == DEFAULT_SERVER) {
            std::cout << "\nСервер кафедры недоступен. Попробуйте:\n";
            std::cout << "  daytime_client --test           # Локальный тестовый сервер\n";
            std::cout << "  daytime_client -s time.nist.gov # Публичный сервер\n";
        }
        return 1;
    }
    
    return 0;
}
