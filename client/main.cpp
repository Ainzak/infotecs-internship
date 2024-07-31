#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cctype>
#include <vector>
#include <algorithm>
#include <chrono>

#define PORT 5152

class SharedBuffer {
public:
    void setData(const std::string& data) {
        std::unique_lock<std::mutex> lock(mtx);
        buffer = data;
        data_ready = true;
        cv.notify_one();
    }
    std::string getData() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return data_ready; });
        std::string data = buffer;
        buffer.clear();
        data_ready = false;
        return data;
    }

private:
    std::string buffer;
    std::mutex mtx;
    std::condition_variable cv;
    bool data_ready = false;
};

bool isValidInput(const std::string& input) {
    if (input.length() > 64) return false;
    for (char c : input) {
        if (!isdigit(c)) return false;
    }
    return true;
}

void processString(std::string& input) {
    std::vector<char> abob;
    for (char& c : input) {
        if ((c - '0') % 2 == 0) {
            abob.push_back('K');
            abob.push_back('B');
        } else {
            abob.push_back(c);
        }
    }
    std::string a(abob.begin(), abob.end());
    input = a;
}

void sortString(std::string& input) {
    std::sort(input.begin(), input.end());
}

void inputThread(SharedBuffer& sharedBuffer) {
    while (true) {
        std::string input;
        std::cout << "Введите строку из цифр: ";
        std::cin >> input;
        if (!isValidInput(input)) {
            std::cout << "Неверный формат ввода" << std::endl;
            continue;
        }
        sortString(input);
        processString(input);
        sharedBuffer.setData(input);
    }
}

void outputThread(SharedBuffer& sharedBuffer) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    while (true) {
        while (true) {
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                std::cout << "Ошибка подключения ip" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                std::cout << "Соединение не удалось. Повтор через 5 секунд" << std::endl;
                close(sock);
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            break;
        }
        while (true) {
            std::string data = sharedBuffer.getData();
            int sum = 0;
            for (char c : data) {
                if (isdigit(c)) {
                    sum += (c - '0');
                }
            }
            std::string sumStr = std::to_string(sum);
            if (send(sock, sumStr.c_str(), sumStr.size(), 0) < 0) {
                std::cout << "Ошибка отправки" << std::endl;
                close(sock);
                break;
            }
        }
    }
}

int main() {
    SharedBuffer sharedBuffer;
    std::thread t1(inputThread, std::ref(sharedBuffer));
    std::thread t2(outputThread, std::ref(sharedBuffer));
    t1.join();
    t2.join();
    return 0;
}
