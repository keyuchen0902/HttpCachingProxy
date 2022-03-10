#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <string>
#include <vector>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <fstream>
#include <list>
#include <utility>
#include <algorithm>
#include "lock.h"
#include "returncode.h"
//#define LOG "proxy.log"//path of log
#define LOG "/var/log/erss/proxy.log" 

//就是干！！ bro
//就是干！！
//final clean up all includes
static int num = 0;
using namespace std;
void parseHeaderline(unordered_map<string,string>& header, string line);
string addtime(string date, string maxage);
//void revalidate(string request,int client_fd, Http_Response cached_resp);
unordered_map<string,string> parseHeader(string headers){
    unordered_map<string, string> results;
    size_t pre_pos = 0;
    size_t cur_pos = 0;
    while(headers.find_first_of("\r\n", pre_pos) != pre_pos){
        //cout<<"this while"<<endl;
        cur_pos = headers.find_first_of("\r\n",pre_pos);
        //cout<<"this line to be parsed is"<<headers.substr(pre_pos, cur_pos - pre_pos)<<endl;
        parseHeaderline(results, headers.substr(pre_pos, cur_pos - pre_pos));
        pre_pos = cur_pos + 2;
    }
    return results;
}
void parseHeaderline(unordered_map<string,string>& header, string line) {
    size_t key_endpos = line.find(':');
    string key = line.substr(0, key_endpos);
    string to_value = line.substr(key_endpos + 1);
    if(to_value == "") {
        //cerr<<"wrong header format, found key but not the value"<<endl;
        return;
        //exit(EXIT_FAILURE);
    }
    int index = 0;
    while(to_value[index] == ' ') {
        index++;
        if(to_value[index] == '\r') {
           // cerr<<"wrong header format, found key but only spaces in the value part"<<endl;
            return;
        }
    }
    string value = to_value.substr(index);
    header[key] = value;
}

class Http_Response{
    public:
    vector<char> buffer;
    string startline;
    string headers;
    string body;
    //need to get from startline 
    string http_version;
    string status_code;
    string reason;
    //need to get from headers
    unordered_map<string, std::string> header;
    bool valid_flag = true;
    //Todo: initialize the buffer member
    Http_Response() {}
    Http_Response(vector<char> resp_buffer) {
        buffer = resp_buffer;
        parse_buffer();
        header = parseHeader(headers);
    }
    void parse_buffer() {
        //logic to give startline, headers, body
        string temp_buffer = buffer.data();
        //get startline
        size_t cur_pos;
        if(temp_buffer.find("\r\n") != string::npos){
            cur_pos = temp_buffer.find("\r\n");
            startline = temp_buffer.substr(0,cur_pos);
        }else{
            return;//no start line
        }
        //cout<<"************"<<endl<<"below is startline"<<endl<<startline<<endl;
        //get headers
        size_t headers_end = temp_buffer.find("\r\n\r\n");
        headers = temp_buffer.substr(cur_pos + 2, headers_end + 2);
        cur_pos = headers_end + 4;
        //get body
        //cout<<"************"<<endl<<"below is headers"<<endl<<headers<<endl;
        body = temp_buffer.substr(cur_pos);
        //cout<<"************"<<endl<<"below is body"<<endl<<body<<endl;
        //make call of subparse functions
        parse_startline();
    }
    void parse_startline() {
        size_t cur_pos;
        cur_pos = startline.find(" ");
        if(cur_pos == string::npos){
            valid_flag = false;
            return;
        }
        http_version = startline.substr(0, cur_pos);
        cur_pos = cur_pos + 1;
        // status_code = startline.substr(cur_pos, startline.find(startline.begin() + cur_pos, startline.end(), " "));
        size_t temp_pos = startline.find(" ", cur_pos);
        if(temp_pos == string::npos){
            valid_flag = false;
            return;
        }
        status_code = startline.substr(cur_pos, temp_pos-cur_pos);
        // cur_pos = startline.find(startline.begin() + cur_pos, startline.end(), " ") + 1;
        cur_pos = startline.find(" ", cur_pos) + 1;
        if(cur_pos == string::npos){
            valid_flag = false;
            return;
        }
        temp_pos = startline.find("\r\n");
        reason = startline.substr(cur_pos, startline.find("\r\n"));
    }

    bool checkExistence(string keyword){
        if(headers.find(keyword) != string::npos){
            return true;
        }else{
            return false;
        }
    }

    void recompose_buffer(){
        vector<char> res;
        string new_headers;
        for(typename unordered_map<string, string>::iterator it = header.begin(); it != header.end(); ++it) {
            new_headers+=it->first;
            new_headers+=":";
            new_headers+=it->second;
            new_headers+="\r\n";
        }
        new_headers+="\r\n";
        headers = new_headers;
        string wholeBuffer = startline + "\r\n" + headers + body;
        for(int i = 0; i < wholeBuffer.length(); i++) {
            res.push_back(wholeBuffer[i]);
        }
        buffer = res;
    }
};
bool isExpired(string date, string max_age);
bool isExpired(string date);
int isExpiration(Http_Response response){
    string max_age;
    if(response.headers.find("Cach-Control") != string::npos){
        if(response.headers.find("Date") != string::npos){
            size_t pos_maxage = response.headers.find("max-age");
            if(pos_maxage!=string::npos){
                size_t pos_col = response.headers.find(",",pos_maxage);
                if(pos_col == string::npos){
                    size_t pos = response.headers.find_first_of("\r\n",pos_maxage);
                    max_age = response.headers.substr(pos_maxage+8,pos-pos_maxage-8);
                }else{
                    max_age = response.headers.substr(pos_maxage+8,pos_col-pos_maxage-8);
                }
                size_t pos_date = response.headers.find("Date");
                size_t pos2 = response.headers.find_first_of("\r\n",pos_date);
                string date = response.headers.substr(pos_date+5,pos2-pos_date-5);

                //string now_date = getTime(date,max_age);
                if(isExpired(date,max_age)){
                    return 1;
                }else{
                    return 2;
                }
            }
        }
    }
    // bool isExpired(string date, string max_age);
    // bool isExpired(string date);
    if(response.headers.find("Expires") != string::npos){
        size_t pos_date = response.headers.find("Expires");
        size_t pos2 = response.headers.find_first_of("\r\n",pos_date);
        string date = response.headers.substr(pos_date+5,pos2-pos_date-5);
        if(isExpired(date)){
                    return 3;
                }else{
                    return 4;
                }
    }
    return 0;
}

bool isExpired(string date, string max_age){
/* difftime example */
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    char str[10];
    char wht[20];
    sscanf(date.data(),"%s %d %s %d %d:%d:%d", wht, &day, str, &year, &hour, &minute, &second);
    string mon_str = str;
    if(mon_str == "Jan") {
        month = 1;
    } else if(mon_str == "Feb") {
        month = 2;
    } else if(mon_str == "Mar") {
        month = 3;
    } else if(mon_str == "Apr") {
        month = 4;
    } else if(mon_str == "May") {
        month = 5;
    } else if(mon_str == "Jun") {
        month = 6;
    } else if(mon_str == "Jul") {
        month = 7;
    } else if(mon_str == "Aug") {
        month = 8;
    } else if(mon_str == "Sep") {
        month = 9;
    } else if(mon_str == "Oct") {
        month = 10;
    } else if(mon_str == "Nov") {
        month = 11;
    } else if(mon_str == "Dec") {
        month = 12;
    }

    time_t now;
    struct tm create_time;
    double seconds;

    time(&now);  /* get current time; same as: now = time(NULL)  */
    create_time = *gmtime(&now);
    create_time.tm_year = year-1900; create_time.tm_hour = hour; create_time.tm_min = minute; create_time.tm_sec = second;
    create_time.tm_mon = month-1;  create_time.tm_mday = day;
    seconds = difftime(now,mktime(&create_time));

    return seconds - stoi(max_age) > 0;
    
}

bool isExpired(string date){
/* difftime example */
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    char str[10];
    char wht[20];
    sscanf(date.data(),"%s %d %s %d %d:%d:%d", wht, &day, str, &year, &hour, &minute, &second);
    string mon_str = str;
    if(mon_str == "Jan") {
        month = 1;
    } else if(mon_str == "Feb") {
        month = 2;
    } else if(mon_str == "Mar") {
        month = 3;
    } else if(mon_str == "Apr") {
        month = 4;
    } else if(mon_str == "May") {
        month = 5;
    } else if(mon_str == "Jun") {
        month = 6;
    } else if(mon_str == "Jul") {
        month = 7;
    } else if(mon_str == "Aug") {
        month = 8;
    } else if(mon_str == "Sep") {
        month = 9;
    } else if(mon_str == "Oct") {
        month = 10;
    } else if(mon_str == "Nov") {
        month = 11;
    } else if(mon_str == "Dec") {
        month = 12;
    }

    time_t now;
    struct tm create_time;
    double seconds;

    time(&now);  /* get current time; same as: now = time(NULL)  */
    create_time = *gmtime(&now);
    create_time.tm_year = year-1900; create_time.tm_hour = hour; create_time.tm_min = minute; create_time.tm_sec = second;
    create_time.tm_mon = month-1;  create_time.tm_mday = day;
    seconds = difftime(now,mktime(&create_time));

    return seconds > 0;
    
}

class Cache{
public:
    typedef list<string> LIST;
    typedef pair<Http_Response,LIST::iterator> PAIR;
    typedef unordered_map<string,PAIR> MAP;

    MAP cache_map;
    LIST old_list;
    size_t cache_size;

    void reOrder(MAP::iterator it){
        old_list.erase(it->second.second);
        old_list.push_front(it->first);
        it->second.second = old_list.begin();
    }


public:
    Cache(size_t size):cache_size(size){};

    bool is_in_cache(string startline){
        if(cache_map.find(startline) != cache_map.end()){
            return true;
        }else{
            return false;
        }
    }

    Http_Response get_cache_response(string startline){
        MAP::iterator it = cache_map.find(startline);
        reOrder(it);
        return it->second.first;
    }

    void put_cache_response(string startline,Http_Response response){
        MAP::iterator it = cache_map.find(startline);
        if(cache_map.find(startline) != cache_map.end()){
            reOrder(it);
        }else{
            if(old_list.size() == cache_size){
                cache_map.erase(old_list.back());
                old_list.pop_back();
            }
            old_list.push_front(startline);
        }
        cache_map[startline] = {response,old_list.begin()};
    }
};

Cache myCache(256);

class Http_Request{
public:
    int id;
    ofstream log;
    vector<char> buffer;
    string startline;
    string headers;
    string body;
    //need to get from startline 
    string method;
    string uri;
    string http_version;
    //need to get from headers
    unordered_map<string, std::string> header;
    //need to get from uri
    string ip;
    string port;
    string hostname;
    //static int num;
    //Todo: initialize the buffer member 5min
    Http_Request(vector<char> req_buffer){
        buffer = req_buffer;
        parse_buffer();
        id = num;
        num++;
        //ios_base::trunc | ios_base::out | ios_base::in);
        log.open(LOG,ios::app);
        header = parseHeader(headers);
    }

    ~Http_Request(){
        log.close();
    }

    void parse_buffer() {
        //logic to give startline, headers, body
        string temp_buffer = buffer.data();
        //get startline
        size_t cur_pos;
        if(temp_buffer.find("\r\n") != string::npos){
            cur_pos = temp_buffer.find("\r\n");
            startline = temp_buffer.substr(0,cur_pos);
        }else{
           // cerr<<"parse_buffer error"<<endl;
            return;//no start line
        }

        //cout<<"************"<<endl<<"below is startline"<<endl<<startline<<endl;

        //get headers
        size_t headers_end = temp_buffer.find("\r\n\r\n");
        headers = temp_buffer.substr(cur_pos + 2, headers_end + 2);
        cur_pos = headers_end + 4;
        //cout<<"************"<<endl<<"below is headers"<<endl<<headers<<endl;
        
        //get body
        body = temp_buffer.substr(cur_pos);
        //make call of subparse functions
        //cout<<"************"<<endl<<"below is body"<<endl<<body<<endl;
        parse_startline();

    }
    void parse_startline() {
        size_t cur_pos;
        cur_pos = startline.find(" ");
        //if not find the method, report 400 error
        method = startline.substr(0, cur_pos);
        //cout<<"***********************"<<endl;
        //cout<<"************"<<endl<<"below is method"<<endl<<method<<endl;

        cur_pos = cur_pos + 1;
        // uri = startline.substr(cur_pos, startline.find(startline.begin() + cur_pos, startline.end(), " "));
        size_t uri_endpos = startline.find_first_of(" ", cur_pos) - 1;
        if(startline[uri_endpos] != '/') {
            uri_endpos += 1;
        }
        if(uri_endpos == string::npos) {
           // cout<<"wrong find"<<endl;
        }
        uri = startline.substr(cur_pos, uri_endpos - cur_pos);

        //cout<<"************"<<endl<<"below is uri"<<endl<<uri<<endl;
        cur_pos = startline.find(" ", cur_pos) + 1;
        http_version = startline.substr(cur_pos, startline.find("\r\n"));

        //cout<<"************"<<endl<<"below is http_version"<<endl<<http_version<<endl;
        parse_uri();
    }

    void parse_uri(){
        string temp_uri = uri;
        size_t pos1 = temp_uri.find("//");
        if(pos1 == string::npos){
            pos1 = 0;
        }else{
            pos1 += 2;
        }
        size_t pos2 = temp_uri.find(":",pos1);
        size_t pos3 = temp_uri.find("/",pos1);
        if(pos3 == string::npos){
            if(pos2 != string::npos){//no '/' have ':'
                //cout<<"&&&&&&&&&&&uri is "<<temp_uri<<endl;
                hostname = temp_uri.substr(pos1,pos2-pos1);
                port = temp_uri.substr(pos2+1);
            }else{//no '/' no ':'
                hostname = temp_uri.substr(pos1);
                if(method == "CONNECT"){
                    port = "443";
                }else{
                    port = "80";
                }
            }
        }else{
            if(pos2 != string::npos){//have '/' and ':'
                hostname = temp_uri.substr(pos1,pos2-pos1);
                port = temp_uri.substr(pos2+1,pos3-pos2);
            }else{//have '/', no ':'
                hostname = temp_uri.substr(pos1,pos3-pos1);
                if(method == "CONNECT"){
                    port = "443";
                }else{
                    port = "80";
                }
            }
        }


    }
   
    void Toget(int client_fd,string client_ip){
        Http_Response response;
        if(myCache.is_in_cache(startline)){
            //fresh?
            //cout<<"incache"<<endl;
            Http_Response cached_resp = myCache.get_cache_response(startline);
            int cache_status = isExpiration(cached_resp);
            //1.compose revalidation request by startline,hostname,"If-Modified-Since","If-None-Match"(if response contains e-tag)
            //2.send request
            //3.recv response
            //4.parse and do things accordingly
            string request;
            request.append(startline);
            request.append("\r\n");
            request.append("Host: ");
            request.append(hostname);
            request.append("\r\n");
            if(cached_resp.header.find("Last-Modified")!= cached_resp.header.end()){
                request.append("If-Modified-since: ");
                request.append(cached_resp.header["Last-Modified"]);
                request.append("\r\n");
            }
            if(cached_resp.header.find("ETag") != cached_resp.header.end()){
                request.append("If-None-Match: ");
                request.append(cached_resp.header["ETag"]);
                request.append("\r\n");
            }
            request+="\r\n";
            /*
            cache_status:
            1: has cache-control:max-age and creation date, expired
            2: has cache-control:max-age and creation date, not expired
            3: no cache-control, has EXPIRED, expired
            4: no cache-control, has EXPIRED, not expired
            0: no cache-control, no EXPIRED
            */
            //if not expired
            if(cache_status == 2 || cache_status == 4) {
               // cout << "if not expired, going here" << endl;
                Lock loglk(&log_mutex);
                log << id << ": in cache, valid"<< endl;
                Lock lk(&cache_mutex);
                myCache.put_cache_response(startline,cached_resp);
                int send_size = send(client_fd, cached_resp.buffer.data(), cached_resp.buffer.size(), 0);
                return;
                //check if has "must-revalidate"
                // if(cached_resp.headers.find("must-revalidate") != string::npos) {
                //     //do revalidation
                //     cout<<"in cache, requires validation(must-revalidate)"<<endl;
                //     log<<id<<": in cache, requires validation"<<endl;
                //     revalidate(request, client_fd, cached_resp);
                // } else {
                //     //cout << "in cache, valid"<<endl;
                //     log << id << ": in cache, valid"<< endl;
                //     myCache.put_cache_response(startline,cached_resp);
                //     int send_size = send(client_fd, cached_resp.buffer.data(), cached_resp.buffer.size(), 0);
                //     return;
                // }
            // if 0, 1, 3, need to revalidate
            } else {
                //cout << "if 0, 1, 3, need to revalidate "<<endl;
                if(cached_resp.headers.find("no-cache") != string::npos){
                    //cout << "in cache, requires validation(no-cache)"<<endl;
                    Lock lk(&log_mutex);
                    log<<id<<": in cache, requires validation"<<endl;
                }else if(cache_status == 0 ){
                    //cout << "in cache, requires validation(cache_status == 0)"<<endl;
                    Lock lk(&log_mutex);
                    log<<id<<": in cache, requires validation"<<endl;
                }else if(cache_status == 1){
                    string seconds = cached_resp.header["max-age"];
                    string date = cached_resp.header["Date"];
                    string expired_time = addtime(date,seconds);
                   // cout << "iin cache, but expired at"<<endl;
                    Lock lk(&log_mutex);
                    log<<id<<": in cache, but expired at "<< expired_time << endl;
                }else if(cache_status == 3){
                    string expired_time = cached_resp.header["Expires"];
                   // cout << "iin cache, but expired at"<<endl;
                    Lock lk(&log_mutex);
                    log<<id<<": in cache, but expired at "<< expired_time << endl;
                }

                revalidate(request, client_fd, cached_resp);
            }
        }else{
            log << id <<": "<<"not in cache"<<endl;
            response = get_response();
            if(response.valid_flag == false){
                return502(client_fd);
                Lock lk(&log_mutex);
                log << id << ": Bad Request"<<endl;
                return; 
            }
            //cout<<response.headers<<endl;
            log << id << ": Received " << "\"" << response.startline <<"\"" <<" from " << hostname << endl; 
            log << id << ": Responding " << "\"" << response.startline <<"\"" <<endl; 
            send(client_fd, response.buffer.data(), response.buffer.size(), 0);
            int second_check = isExpiration(response);
            if(response.checkExistence("no-store")){
                Lock lk(&log_mutex);
                log << id << ": not cacheable becasue no-store in header" <<endl;

            }else if(response.checkExistence("max-age=0")){
                Lock lk(&log_mutex);
                //<<"maxage = 0"<<endl;
                log << id << ": not cacheable because max-age=0"<<endl;
            }
            else if(response.checkExistence("private")){
                Lock lk(&log_mutex);
                log << id << ": not cacheable becasue private in header" <<endl;
            }else if(response.checkExistence("no-cache") || second_check == 3){
                Lock lk(&log_mutex);
                log << id << ": cached, but requires re-validation" <<endl;
                myCache.put_cache_response(startline,response);
            }
            else if(response.checkExistence("max-age")){ 
                if(response.checkExistence("Date")){
                    string date = response.header["Date"];
                    string seconds = response.header["max-age"];
                    string expires_time = addtime(date,seconds);
                    Lock lk(&log_mutex);
                    log << id << ": cached, expires at "<< expires_time << endl;
                }
                myCache.put_cache_response(startline,response);
            }else if(response.checkExistence("Expires")){
                string expired_time = response.header["Expires"];
                //cout << "cached, expires at" << endl;
                Lock lk(&log_mutex);
                log << id << ": cached, expires at "<< expired_time << endl;

                myCache.put_cache_response(startline,response);
            }/*else if(response.checkExistence("must-revalidate")){
                cout  << "cached, but requires re-validation" << endl;
                log << id << ": cached, but requires re-validation"<<endl;
                myCache.put_cache_response(startline,response);
            }else{
                cout<<"2"<<endl;
                myCache.put_cache_response(startline,response);
            }*/

        }
    }
    
    void Topost(int client_fd){
        Http_Response response;
        response = get_response();
        send(client_fd, response.buffer.data(), response.buffer.size(), 0);
    }

    void Toconnect(int client_fd) {
        int status;
        int server_fd;
        struct addrinfo host_info;
        struct addrinfo* host_info_list;
        const char* host_name = hostname.c_str();
        const char* port = "443";
        memset(&host_info, 0, sizeof(host_info));

        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        //host_info.ai_flags = AI_PASSIVE;

        status = getaddrinfo(host_name, port, &host_info, &host_info_list);
        if (status != 0) {
            //cerr << "Error: cannot get address info for host" << endl;
            //cerr << "  (" << host_name << "," << port << ")" << endl;
            return;
            //exit(EXIT_FAILURE);
        }
        //create a socket
        server_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
        if (server_fd == -1) {
            cerr << "Error: cannot create socket for connecting to server" << endl;
            cerr << "  (" << host_name << "," << port << ")" << endl;
            return;
            exit(EXIT_FAILURE);
        }
        status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to server" << endl;
            cerr << "  (" << host_name << "," << port << ")" << endl;
            return;
            exit(EXIT_FAILURE);
        }
        //response to client
         //send 200 OK info back to browser
        std::string sendOK("HTTP/1.1 200 OK\r\n\r\n");
        int sendConnect = send(client_fd, sendOK.c_str(), strlen(sendOK.c_str()), 0);
        //cout<<"sending 200 OK back to client when establishing the first tunnel"<<endl;
        if (sendConnect < 0)
        {
            std::cerr << "Error: connection fail" << std::endl;
            return;
            exit(EXIT_FAILURE);
        }
        //after established connection with original server
        //start monitoring incoming informatin from client and server using select
        fd_set readfds;
        vector<int> connection_fds;
        connection_fds.push_back(client_fd);
        connection_fds.push_back(server_fd);
        int to_nfds = *max_element(connection_fds.begin(), connection_fds.end()) + 1;
        
        while(true) {
            //cout<<"waiting"<<endl;
            FD_ZERO(&readfds);
            for(int i = 0; i < connection_fds.size(); i++) {
                FD_SET(connection_fds[i], &readfds);
            }
            int target_fd;
            int target_id;
            vector<char> recv_data(40960);
            //fill(recv_data.begin(), recv_data.end(),'\0');
            //monitor reading
            int select_status = select(to_nfds, &readfds, NULL, NULL, NULL);//change
            if(select_status == -1) {
                cerr<<"fail selecting in connect"<<endl;
                return;
                exit(EXIT_FAILURE);
            }
            for(int i = 0; i < connection_fds.size(); i++) {
            //when to break the whole loop???
                if(FD_ISSET(connection_fds[i], &readfds)) {
                    fill(recv_data.begin(), recv_data.end(), '\0');
                    //recv_data = receive_data(connection_fds[i]);
                    target_id = (i + 1) % 2;
                    int recvdata_size = recv(connection_fds[i], recv_data.data(), 40960, 0);
                    if(recvdata_size < 0) {
                        cerr<<"fail to receive data from source"<<endl;
                        //threads
                        return;
                        //exit(EXIT_FAILURE);
                    }
                    
                    if(recvdata_size == 0) {
                        Lock lk(&cache_mutex);
                        log << id << ": Tunnel Closed" <<endl;
                        close(server_fd);
                        return;
                    }
                    //recv(connection_fds[i], recv_data.data(), 4096, 0);
                    target_fd = connection_fds[(i + 1) % 2];
                    recv_data.push_back('\0');
                    recv_data.pop_back();
                    int new_status = send(target_fd, recv_data.data(), recvdata_size, 0);
                    if(new_status < 0) {
                        cerr<<"cannot transfer message to target"<<endl;
                        return;
                    }
                    break;
                }
            }
            
        }
        
    }
    //get response from server for GET request
    Http_Response get_response() {
        log << id << ": Requesting "<<"\"" <<startline<<"\" " << "from" << hostname << endl;
        int status;
        int server_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *host_name = hostname.c_str();
        const char *portnum = port.c_str();
    

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(host_name, portnum, &host_info, &host_info_list);
        struct sockaddr_in* addr = (struct sockaddr_in * )host_info_list->ai_addr;
        //cout<<"original server ip is "<<inet_ntoa(addr->sin_addr)<<endl;

        if (status != 0) {
            //cerr << "Error: cannot get address info for host" << endl;
            //cerr << "  (" << host_name << "," << portnum << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if

        server_fd = socket(host_info_list->ai_family, 
                    host_info_list->ai_socktype, 
                    host_info_list->ai_protocol);
        if (server_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            cerr << "  (" << host_name << "," << portnum << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if
        
        //cout << "Connecting to " << host_name << " on port " << portnum << "..." << endl;
        
        status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            cerr << "  (" << host_name << "," << port << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if
        send(server_fd, buffer.data(), buffer.size(), 0);
        //cout<<"send to original server finished"<<endl;
        vector<char> received_from_serve = receive_data(server_fd);
        //cout<<"response from original server received"<<endl;
        freeaddrinfo(host_info_list);
        close(server_fd);
        return Http_Response(received_from_serve);
    }

    Http_Response get_revalidate_response(string revalidation_request,int client_fd){
        int status;
        int server_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *host_name = hostname.c_str();
        const char *portnum = port.c_str();
    
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(host_name, portnum, &host_info, &host_info_list);
        struct sockaddr_in* addr = (struct sockaddr_in * )host_info_list->ai_addr;
        //cout<<"original server ip is "<<inet_ntoa(addr->sin_addr)<<endl;
        //cout<<"_____________________"<<endl;
        if (status != 0) {
            //cerr << "Error: cannot get address info for host" << endl;
            //cerr << "  (" << host_name << "," << portnum << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if

        server_fd = socket(host_info_list->ai_family, 
                    host_info_list->ai_socktype, 
                    host_info_list->ai_protocol);
        if (server_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            cerr << "  (" << host_name << "," << portnum << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if
        
        //cout << "Connecting to " << host_name << " on port " << portnum << "..." << endl;
        
        status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            cerr << "  (" << host_name << "," << port << ")" << endl;
            return Http_Response();
            //exit(EXIT_FAILURE);
        } //if

        //cout<<"revalidation_request is "<<revalidation_request<<endl;
        send(server_fd, revalidation_request.c_str(), revalidation_request.length(), 0);
        //cout<<"send revalidatino request to original server finished"<<endl;
        vector<char> received_from_serve = receive_data(server_fd);
        //cout<<"response from original server received"<<endl;
        freeaddrinfo(host_info_list);
        close(server_fd);
        return Http_Response(received_from_serve);
    }

    void revalidate(string request,int client_fd, Http_Response cached_resp){
        Http_Response revalidation_response;
        revalidation_response = get_revalidate_response(request,client_fd);
        if(revalidation_response.valid_flag == false){
            return502(client_fd);
            Lock lk(&cache_mutex);
            log << id << ": Bad Request"<<endl;
            return; 
        }
        log << id << ": Received " << "\"" << revalidation_response.startline <<"\"" <<" from " << hostname << endl;
        log << id << ": Responding " << "\"" << revalidation_response.startline <<"\"" <<endl;  
        //cout << revalidation_response.status_code << endl;
        if(revalidation_response.status_code == "304") {
            //cout << "entering 304" << endl;
            if(revalidation_response.header.find("Last-Modified") != revalidation_response.header.end()) {
                cached_resp.header["Last-Modified"] = revalidation_response.header["Last-Modified"];
            }
            if(revalidation_response.header.find("Etag") != revalidation_response.header.end()) {
                cached_resp.header["Etag"] = revalidation_response.header["Etag"];
            }
            cached_resp.recompose_buffer();
            //Todo:function to recompose buffer zone of the response
            myCache.put_cache_response(startline, cached_resp);
            int send_size = send(client_fd, cached_resp.buffer.data(), cached_resp.buffer.size(), 0);
            //send cache response back to browser
            //update the sequence of the response in cache

        } else if (revalidation_response.status_code == "200") {
            myCache.put_cache_response(startline, revalidation_response);
            //<<"new response acquired and sent back and cached in"<<endl;
            int send_size = send(client_fd, revalidation_response.buffer.data(), revalidation_response.buffer.size(), 0);
            return;
            //delete old response
            //add this response to the cached map
        } else {
            //cerr<<"bad response from server during revalidation"<<endl;
            return;
            exit(EXIT_FAILURE);
        }
    }
};

string addtime(string date, string maxage){
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    char str[10];
    char wht[20];
    sscanf(date.data(),"%s %d %s %d %d:%d:%d", wht, &day, str, &year, &hour, &minute, &second);
    string mon_str = str;
    if(mon_str == "Jan") {
        month = 1;
    } else if(mon_str == "Feb") {
        month = 2;
    } else if(mon_str == "Mar") {
        month = 3;
    } else if(mon_str == "Apr") {
        month = 4;
    } else if(mon_str == "May") {
        month = 5;
    } else if(mon_str == "Jun") {
        month = 6;
    } else if(mon_str == "Jul") {
        month = 7;
    } else if(mon_str == "Aug") {
        month = 8;
    } else if(mon_str == "Sep") {
        month = 9;
    } else if(mon_str == "Oct") {
        month = 10;
    } else if(mon_str == "Nov") {
        month = 11;
    } else if(mon_str == "Dec") {
        month = 12;
    }

    time_t now;
    struct tm create_time;
    double seconds;

    time(&now);  /* get current time; same as: now = time(NULL)  */
    create_time = *gmtime(&now);
    create_time.tm_year = year - 1900; create_time.tm_hour = hour; create_time.tm_min = minute; create_time.tm_sec = second;
    create_time.tm_mon = month - 1;  create_time.tm_mday = day;
    now = timegm(&create_time);
    now += stoi(maxage);
    create_time = *gmtime(&now);
    return asctime(&create_time);
}