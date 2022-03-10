#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <vector>

using namespace std;
void request_handler(int socket_fd, string client_ip);
vector<char> receive_data(int socket_fd);
//int isExpiration(Http_Response response);
bool isExpired(string date, string max_age);
bool isExpired(string date);