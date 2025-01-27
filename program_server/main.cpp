#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "functions.h"

using namespace std;

const int BUFFER_SIZE = 4096;
const int PORT = 8080;
const int MAX_CONN_ATTEMPTS = 5;

void handle_client(int client_sock) {
    vector<char> buffer(BUFFER_SIZE);
    string received_data;
    
    try {
        while(true) {
            ssize_t bytes_read = recv(client_sock, buffer.data(), buffer.size(), 0);
            
            if(bytes_read > 0) {
                received_data.append(buffer.data(), bytes_read);
            } 
            else if(bytes_read == 0) {
                break;
            }
            else {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                throw runtime_error("recv error: " + string(strerror(errno)));
            }
        }

        bool validation_result = function3(received_data);
        
        cout << "\n=== Новое сообщение ===" << endl;
        cout << "Длина: " << received_data.length() << endl;
        cout << "Требования: " << (validation_result ? "Соблюдены" : "Несоблюдены") << endl;
        cout << "Содержание: " << (validation_result ? received_data : "ОШИБКА") << endl;
        cout << "===================\n" << endl;

    } catch(const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
    }

    close(client_sock);
}

void setup_server_socket(int& server_fd) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        throw runtime_error("Ошибка создания сокета");
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        throw runtime_error("Ошибка привязки сокета");
    }

    if(listen(server_fd, 5) < 0) {
        throw runtime_error("Ошибка прослушивания порта");
    }

    // Set non-blocking mode for accept
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
}

int main() {
    int server_fd;
    int connection_attempts = 0;

    try {
        setup_server_socket(server_fd);
        cout << "Сервер запущен на порту " << PORT << endl;
        cout << "Ожидание подключений..." << endl;

        while(true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_sock = accept(server_fd, (sockaddr*)&client_addr, &client_len);

            if(client_sock >= 0) {
                connection_attempts = 0;
                cout << "Новое подключение от "
                     << inet_ntoa(client_addr.sin_addr) << endl;

                timeval timeout{.tv_sec = 2, .tv_usec = 0};
                setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

                handle_client(client_sock);
            }
            else {
                if(errno != EWOULDBLOCK) {
                    cerr << "Ошибка подключения: " << strerror(errno) << endl;
                    if(++connection_attempts > MAX_CONN_ATTEMPTS) {
                        throw runtime_error("Слишком много ошибок подключения");
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
    } 
    catch(const exception& e) {
        cerr << "Критическая ошибка: " << e.what() << endl;
        if(server_fd >= 0) close(server_fd);
        return EXIT_FAILURE;
    }

    close(server_fd);
    return EXIT_SUCCESS;
}