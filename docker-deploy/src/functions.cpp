#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <string>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <cstdlib>
#include "functions.h"
#include "httpmodel.h"
#include <stdio.h>      /* printf */
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <utility>
using namespace std;
#define LOG "/var/log/erss/proxy.log" 
//#define LOG "proxy.log" 

void request_handler(int socket_fd, string client_ip){
    //string client_ip = "";
    try{
    ofstream log(LOG, ios::app);
    vector<char> received = receive_data(socket_fd);
    Http_Request http_request(received);
    //print ID:"Request" from IPDROM@TIME upon receiving a new request
    time_t now;
    struct tm *nowdate;
    time(&now);
    nowdate = gmtime(&now);
    log << http_request.id << ": "<<"\"" << http_request.startline << "\"" << " from "<<client_ip << "@" << asctime(nowdate);
    if(http_request.method == "GET"){
        http_request.Toget(socket_fd,client_ip);
        //http_request.Topost(socket_fd);
    //if the request method is connect
    } else if(http_request.method == "CONNECT") {
        http_request.Toconnect(socket_fd);
    } else if(http_request.method == "POST") {
        http_request.Topost(socket_fd);
    }else{
        //cout<<"request_handler error"<<endl;
        Lock lk(&log_mutex);
        return400(socket_fd);
        log<< http_request.id <<"HTTP/1.1 400 Bad Request"<<endl;
    }
    close(socket_fd);
    } catch (exception& e){

    }
    
}
//different
/*bool Is_Last_Chunk(vector<char> buffer) {
    buffer.push_back('\0');
    string temp = buffer.data();
    size_t end_pos = temp.find("0\r\n\r\n");
    if(end_pos == string::npos) {
        return false;
    } else {
        return true;
    }
}

vector<char> deal_chunck_transmission(int socket_fd,vector<char> buffer){
    vector<char> res;
    vector<char> temp(33331);
    res = buffer;
    while(!Is_Last_Chunk(res)) {
        cout<<"dealing with chunk"<<endl;
        cout<<"-------------------"<<endl;
        vector<char> buf = res;
        buf.push_back('\0');
        string str = buf.data();
        cout<<str<<endl;
        cout<<"-------------------"<<endl;
        int temp_size = recv(socket_fd, temp.data(), 33331, 0);
        if(temp_size < 0) {
            cerr<<"error when receiving data segment"<<endl;
            exit(EXIT_FAILURE);
        } else if(temp_size == 0) {
            res.push_back('\0');
            cout<<"temp_size = 0"<<endl;
            continue;
        }
        for(int i = 0; i < temp_size; i++) {
            res.push_back(temp[i]);
        }
        fill(temp.begin(),temp.end(), 0);
    }
    //bool is_last_chunk =  Is_Last_Chunk(buffer);
    // buffer.push_back('\0');
    // for(int i = 0; buffer[i] != '\0'; i++) {
    //     res.push_back(buffer[i]);        
    // }
    res.push_back('\0');
    return res;
}*/
//#######################need change#######################
size_t Is_Last_Chunk(vector<char> buffer) {
    buffer.push_back('\0');
    string temp = buffer.data();
    //
    // cout<<"this chunk is"<<temp<<endl;
    //
    size_t end_pos = temp.find("0\r\n\r\n");
    if(end_pos == string::npos) {
        return 0;
    }

    return end_pos+5;
  
}

vector<char> deal_chunck_transmission(int socket_fd,vector<char> buffer){
    vector<char> res;
    vector<char> temp(33331);
    size_t pos = Is_Last_Chunk(buffer);
    if(pos != 0){
        for(size_t i =0; i < pos;i++){
            res.push_back(buffer[i]);//???
        }
        res.push_back('\0');

        return res;
    }else{
        res = buffer;
    }
    //
    // buffer.push_back('\0');
    // string out = buffer.data();
    // cout<<"first chunk is"<< endl<<out<<endl;
    while(1) {
        //cout<<"dealing with chunk"<<endl;
        int temp_size = recv(socket_fd, temp.data(), 33331, 0);
        if(temp_size < 0) {
            //cerr<<"error when receiving data segment"<<endl;
            return vector<char>(1,0);
            exit(EXIT_FAILURE);
        } 
        pos = Is_Last_Chunk(temp);
        if(pos!=0){
            for(size_t i = 0; i < pos;i++){
                res.push_back(temp[i]);
            }
            res.push_back('\0');
            return res;
        }else{
            for(size_t i = 0; i < temp_size;i++){
                res.push_back(temp[i]);
            }
        }
        //
        // res.push_back('\0');
        // string out = res.data();
        // cout<<"chunk is"<< endl<<out<<endl;
        // res.pop_back();
        //
        if(temp_size == 0){
            res.push_back('\0');
            return res;
        }
        fill(temp.begin(),temp.end(), '\0');
    }
}
//####################################################################

vector<char> receive_data(int socket_fd) {
    vector<char> buffer;
    vector<char> temp(33331, 0);
    int i = 0;
    while(true) {
        //cout<<"receiving data segment "<<++i<<endl;
        //int temp_size = recv(socket_fd, &temp.data()[0], 4096, 0);
        int temp_size = recv(socket_fd, temp.data(), 33331, 0);
        if(temp_size < 0) {
           // cerr<<"error when receiving this data segment"<<endl;
            close(socket_fd);
            return vector<char>(1,0);
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < temp_size;i++){
            buffer.push_back(temp[i]);
        }
        fill(temp.begin(),temp.end(),'\0');
        //for empty request
        if(temp_size == 0) {
            buffer.push_back('\0');
            break;//change
        }
        if(temp_size < temp.size()) {
            //check if the end of chunk transmission
            vector<char> buffer_copy = buffer;
            buffer_copy.push_back('\0');
            string request_lines = buffer_copy.data();
            //check chunk,0 is not chunk, 1 is chunk
            int flag = 0;
            size_t start = request_lines.find("Transfer-Encoding");
            if(start != string::npos){
                if(request_lines.find("chunked",start) != string::npos){
                    flag = 1;
                }
            }
            if(flag == 1){
                //<<"going heree"<<endl;
                //needs to do something further for chunk transmission
                return deal_chunck_transmission(socket_fd, buffer);
            }
            //finish receive, responding.
            buffer.push_back('\0');
            //to do: write response class,whether need to check no chunk
            Http_Response mid_res(buffer);
            if(mid_res.headers.find("Content-Length") == string::npos){
                break;
            }
            //cout<<"##########find content-length##########"<<endl;
            size_t pos1 = mid_res.headers.find("Content-Length");
            size_t pos2 = mid_res.headers.find_first_of(":",pos1);
            size_t pos3 = mid_res.headers.find_first_of("\r\n",pos2);
            string len_val = mid_res.headers.substr(pos2+1,pos3-pos2);
            if(mid_res.body.size() >= stoi(len_val) - 1){
                break;
            }else{
                buffer.pop_back();
                return buffer;
            }                
            
            
        }
    }
    return buffer;
}

