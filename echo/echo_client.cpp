#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

const int DEFAULT_PORT = 7;
const std::string DEFAULT_SERVER = "172.16.40.1";
const int BUFFER_SIZE = 1024;

void show_help() {
    std::cout << "Клиент службы Echo (TCP)\n";
    std::cout << "Использование: echo_client [-s сервер] [-p порт] [-h] [--test] \"сообщение\"\n";
    std::cout << "Параметры:\n";
    std::cout << "  -s, --server АДРЕС   IP-адрес или доменное имя сервера (по умолчанию: 172.16.40.1)\n";
    std::cout << "  -p, --port ПОРТ      Номер порта (по умолчанию: 7)\n";
    std::cout << "  -h, --help          Показать эту справку\n";
    std::cout << "  --test              Использовать тестовый сервер (localhost:1707)\n";
    std::cout << "  сообщение           Текст для отправки на сервер\n";
    std::cout << "Примеры:\n";
    std::cout << "  echo_client \"Hello\"                    # Сервер кафедры\n";
    std::cout << "  echo_client --test \"Test Message\"      # Локальный тестовый сервер\n";
    std::cout << "  echo_client -s localhost -p 1707 \"Hi\" # Ручное указание тестового сервера\n";
    std::cout << "  echo_client -s tcpbin.com -p 4242 \"Hello\" # Публичный сервер\n";
}

bool resolve_hostname(const std::string& hostname, std::string& ip_address) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    
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

bool connect_to_echo_server(const std::string& server, int port, const std::string& message) {
    // Создание TCP сокета
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return false;
    }
    
    // Установка таймаута
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
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
    
    // Подключение к серверу
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка подключения к серверу\n";
        close(sockfd);
        return false;
    }
    
    std::cout << "Успешно подключено к серверу\n";
    
    // Отправка сообщения
    ssize_t sent_len = send(sockfd, message.c_str(), message.length(), 0);
    if (sent_len < 0) {
        std::cerr << "Ошибка отправки сообщения\n";
        close(sockfd);
        return false;
    }
    
    std::cout << "Отправлено: " << message << "\n";
    
    // Получение ответа
    char buffer[BUFFER_SIZE];
    ssize_t recv_len = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    
    if (recv_len < 0) {
        std::cerr << "Ошибка получения ответа\n";
        close(sockfd);
        return false;
    }
    
    buffer[recv_len] = '\0';
    std::cout << "Получено: " << buffer << "\n";
    
    close(sockfd);
    return true;
}

int main(int argc, char* argv[]) {
    std::string server = DEFAULT_SERVER;
    int port = DEFAULT_PORT;
    std::string message;
    bool test_mode = false;
    
    if (argc < 2) {
        show_help();
        return 0;
    }
    
    // Разбор аргументов командной строки
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help();
            return 0;
        } else if (arg == "--test") {
            test_mode = true;
            server = "localhost";
            port = 1707;
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
        } else {
            // Аргумент без флага - сообщение
            message = arg;
        }
    }
    
    if (message.empty()) {
        std::cerr << "Ошибка: не указано сообщение для отправки\n";
        show_help();
        return 1;
    }
    
    if (test_mode) {
        std::cout << "=== ТЕСТОВЫЙ РЕЖИМ ===\n";
    }
    
    // Попытка подключения к серверу
    if (!connect_to_echo_server(server, port, message)) {
        if (!test_mode && server == DEFAULT_SERVER) {
            std::cout << "\nСервер кафедры недоступен. Попробуйте:\n";
            std::cout << "  echo_client --test \"Ваше сообщение\"     # Локальный тестовый сервер\n";
            std::cout << "  echo_client -s localhost -p 1707 \"Сообщение\"\n";
            std::cout << "  echo_client -s tcpbin.com -p 4242 \"Сообщение\" # Публичный сервер\n";
        }
        return 1;
    }
    
    return 0;
}
