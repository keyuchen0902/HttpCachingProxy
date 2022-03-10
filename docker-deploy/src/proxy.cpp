#include <exception>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <functional>
#include "functions.h"
using namespace std;
//#define LOG "/var/log/erss/proxy.log"
std::ofstream log("/var/log/erss/proxy.log");
//int Http_Request::num = 0;
int main(int argc, char* argv[]){
    /*ofstream log;
    log.open(LOG);
    log.close();*/
    int status;
    int sockfd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    char hostname[256];
    memset(hostname, 0, sizeof(hostname));
    const char *port = "12345";
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        cout << "Hostname access fail" << endl;
        exit(EXIT_FAILURE);
    }
    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } // if

    int server_proxy_fd = socket(host_info_list->ai_family,
                            host_info_list->ai_socktype,
                            host_info_list->ai_protocol);
    if (server_proxy_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } // if

    int yes = 1;
    status = setsockopt(server_proxy_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(server_proxy_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } // if

    status = listen(server_proxy_fd, 100);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    while(1){
        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd;
        client_connection_fd = accept(server_proxy_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        
        if (client_connection_fd == -1) {
            cerr << "Error: cannot accept connection on socket" << endl;
            //exit
            //return -1;

        } //if
        struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
        string client_ip = inet_ntoa(addr->sin_addr);
        //ta args(client_connection_fd, client_ip);
        thread newthread(request_handler, client_connection_fd, client_ip);
        newthread.detach();
        //request_handler(client_connection_fd);

    }

    freeaddrinfo(host_info_list);
    close(server_proxy_fd);

    return 0;//最后全改exit 
}