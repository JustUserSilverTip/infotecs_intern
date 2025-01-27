#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "functions.h"

using namespace std;


mutex mtx_console;    
mutex mtx_buffer;     
mutex mtx_input;      
condition_variable cv_buffer;
condition_variable cv_input;
string shared_buffer;
bool data_ready = false;
bool input_allowed = true;

void input_thread() {
    string input;
    while(true) {
        {
            unique_lock<mutex> lock_input(mtx_input);
            cv_input.wait(lock_input, []{ return input_allowed; });
        }

        {
            lock_guard<mutex> lock_console(mtx_console);
            cout << "Введите цифры (макс. 64 символа): ";
        }

        if(!(cin >> input)) {
            lock_guard<mutex> lock_console(mtx_console);
            cout << "\nЗавершение работы программы" << endl;
            exit(0);
        }

        bool validation_error = false;
        if(input.size() > 64) {
            lock_guard<mutex> lock_console(mtx_console);
            cout << "Ошибка: превышена максимальная длина (64 символа)" << endl;
            validation_error = true;
        }
        else if(!all_of(input.begin(), input.end(), ::isdigit)) {
            lock_guard<mutex> lock_console(mtx_console);
            cout << "Ошибка: обнаружены недопустимые символы" << endl;
            validation_error = true;
        }

        if(validation_error) {
            {
                lock_guard<mutex> lock(mtx_input);
                input_allowed = true;
            }
            cv_input.notify_one();
            continue;
        }

        function1(input);

        {
            lock_guard<mutex> lock_buffer(mtx_buffer);
            shared_buffer = move(input);
            data_ready = true;
            input_allowed = false;
        }
        cv_buffer.notify_one();
    }
}

bool send_to_server(const string& data) {
    const int max_retries = 3;
    constexpr chrono::milliseconds retry_interval{1000};
    int attempts = 0;

    while(attempts++ < max_retries) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) {
            lock_guard<mutex> lock(mtx_console);
            cerr << "Ошибка создания сокета" << endl;
            continue;
        }

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(8080);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

        timeval timeout{.tv_sec = 1, .tv_usec = 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if(connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0) {
            ssize_t sent = send(sock, data.c_str(), data.size(), 0);
            close(sock);

            if(sent == static_cast<ssize_t>(data.size())) {
                return true;
            }

            lock_guard<mutex> lock(mtx_console);
            cerr << "Частичная отправка: " << sent << "/" << data.size() << endl;
        } else {
            close(sock);
            lock_guard<mutex> lock(mtx_console);
            cerr << "Ошибка соединения (попытка " << attempts << "/" << max_retries << ")" << endl;
        }

        if(attempts < max_retries) {
            this_thread::sleep_for(retry_interval);
        }
    }

    lock_guard<mutex> lock(mtx_console);
    cerr << "Не удалось отправить данные после " << max_retries << " попыток" << endl;
    return false;
}

void processing_thread() {
    while(true) {
        unique_lock<mutex> lock_buffer(mtx_buffer);
        cv_buffer.wait(lock_buffer, []{ return data_ready; });

        string data = move(shared_buffer);
        shared_buffer.clear();
        data_ready = false;
        lock_buffer.unlock();

        {
            lock_guard<mutex> lock_console(mtx_console);
            cout << "Обработанные данные: " << data << endl;
            cout << "Сумма чисел: " << function2(data) << endl;
        }

        bool send_result = send_to_server(data);

        {
            lock_guard<mutex> lock(mtx_input);
            input_allowed = true;
        }
        cv_input.notify_one();

        if(!send_result) {
            lock_guard<mutex> lock(mtx_console);
            cerr << "Внимание: данные не были доставлены в program_server" << endl;
        }
    }
}

int main() {
    thread input_worker(input_thread);
    thread processing_worker(processing_thread);

    input_worker.join();
    processing_worker.join();

    return 0;
}