#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <mutex>
#include "concurrency.hpp"

using namespace std;

void handleclient(int clientfd) {
    {
        lock_guard<mutex> lock(clients_lock);
        active_clients.push_back(clientfd);
    }
    char buffer[64] = {0};
    ssize_t bytes_read;
    while ((bytes_read = read(clientfd, buffer, sizeof(buffer))) > 0) {
        pushring(string(buffer, bytes_read));
    }
    {
        lock_guard<mutex> lock(clients_lock);
        active_clients.erase(remove(active_clients.begin(), active_clients.end(), clientfd), active_clients.end());
    }
    close(clientfd);
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    
    thread consumer(consumerloop);
    consumer.detach();

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(serverfd, (sockaddr*)&address, sizeof(address));
    listen(serverfd, maxclients);

    while (true) {
        int clientfd = accept(serverfd, nullptr, nullptr);
        if (clientfd >= 0) {
            thread clientthread(handleclient, clientfd);
            clientthread.detach();
        }
    }

    return 0;
}
