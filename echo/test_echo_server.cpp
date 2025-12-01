#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <vector>

const int ECHO_PORT = 1707;
const int BUFFER_SIZE = 1024;
const int MAX_CLIENTS = 10;

bool running = true;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nЗавершение работы сервера...\n";
        running = false;
    }
}

void handle_client(int client_sock, struct sockaddr_in client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    std::cout << "Обработка клиента " << client_ip << ":" << client_port << std::endl;
    
    char buffer[BUFFER_SIZE];
    
    while (running) {
        // Получение данных от клиента
        ssize_t recv_len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (recv_len <= 0) {
            break;
        }
        
        buffer[recv_len] = '\0';
        std::string message(buffer);
        
        std::cout << "Получено от " << client_ip << ":" << client_port << ": " << message;
        
        // Отправка эхо-ответа
        ssize_t sent_len = send(client_sock, buffer, recv_len, 0);
        
        if (sent_len < 0) {
            std::cerr << "Ошибка отправки эхо-ответа\n";
            break;
        } else {
            std::cout << "Отправлено эхо: " << message;
        }
    }
    
    close(client_sock);
    std::cout << "Клиент " << client_ip << ":" << client_port << " отключен\n";
}

int main() {
    // Установка обработчика сигнала
    signal(SIGINT, signal_handler);
    
    // Создание TCP сокета
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }
    
    // Установка опции повторного использования адреса
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Ошибка установки SO_REUSEADDR\n";
        close(server_sock);
        return 1;
    }
    
    // Настройка адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(ECHO_PORT);
    
    // Привязка сокета к адресу
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка привязки сокета\n";
        close(server_sock);
        return 1;
    }
    
    // Прослушивание порта
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        std::cerr << "Ошибка прослушивания порта\n";
        close(server_sock);
        return 1;
    }
    
    std::cout << "Echo сервер запущен на порту " << ECHO_PORT << std::endl;
    std::cout << "Для завершения работы нажмите Ctrl+C\n\n";
    
    std::vector<std::thread> client_threads;
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Принятие соединения
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock < 0) {
            if (running) {
                std::cerr << "Ошибка принятия соединения\n";
            }
            continue;
        }
        
        // Запуск потока для обработки клиента
        client_threads.emplace_back(handle_client, client_sock, client_addr);
    }
    
    // Закрытие серверного сокета
    close(server_sock);
    
    // Ожидание завершения всех клиентских потоков
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "Сервер завершил работу\n";
    return 0;
}
