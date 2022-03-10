#include <string>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <cstdlib>

using namespace std;

void return400(int client_fd){
   string bad_request = "HTTP/1.1 400 Bad Request";
   send(client_fd,bad_request.c_str(),bad_request.size(),MSG_NOSIGNAL);
}

void return502(int client_fd){
   string bad_gateway = "HTTP/1.1 502 Bad Gateway";
   send(client_fd,bad_gateway.c_str(),bad_gateway.size(),MSG_NOSIGNAL);
}
