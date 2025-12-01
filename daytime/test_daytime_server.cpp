#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

const int DAYTIME_PORT = 1313;
const int BUFFER_SIZE = 1024;

bool running = true;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nЗавершение работы сервера...\n";
        running = false;
    }
}

std::string get_current_time() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d %b %Y %H:%M:%S MSK", timeinfo);
    
    return std::string(buffer);
}

int main() {
    // Установка обработчика сигнала для корректного завершения
    signal(SIGINT, signal_handler);
    
    // Создание UDP сокета
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }
    
    // Настройка адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DAYTIME_PORT);
    
    // Привязка сокета к адресу
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка привязки сокета\n";
        close(sockfd);
        return 1;
    }
    
    std::cout << "Daytime сервер запущен на порту " << DAYTIME_PORT << std::endl;
    std::cout << "Для завершения работы нажмите Ctrl+C\n\n";
    
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (running) {
        // Ожидание запроса от клиента
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                   (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len < 0) {
            if (running) {
                std::cerr << "Ошибка получения данных\n";
            }
            continue;
        }
        
        buffer[recv_len] = '\0';
        
        // Получение информации о клиенте
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        
        std::cout << "Получен запрос от " << client_ip << ":" << client_port << std::endl;
        
        // Формирование ответа с текущим временем
        std::string current_time = get_current_time() + "\n";
        
        // Отправка времени клиенту
        ssize_t sent_len = sendto(sockfd, current_time.c_str(), current_time.length(), 0,
                                 (struct sockaddr*)&client_addr, client_len);
        
        if (sent_len < 0) {
            std::cerr << "Ошибка отправки ответа\n";
        } else {
            std::cout << "Отправлено время: " << current_time;
        }
    }
    
    close(sockfd);
    std::cout << "Сервер завершил работу\n";
    return 0;
}
