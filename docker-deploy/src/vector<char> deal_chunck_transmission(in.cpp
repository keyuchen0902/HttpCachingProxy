size_t Is_Last_Chunk(vector<char> buffer) {
    buffer.push_back('\0');
    string temp = buffer.data();
    size_t end_pos = temp.find("0\r\n\r\n");
    if(end_pos == string::npos) {
        return 0;
    } else {
        return end_pos+5;
    }
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

    while(1) {
        cout<<"dealing with chunk"<<endl;
        int temp_size = recv(socket_fd, temp.data(), 33331, 0);
        if(temp_size < 0) {
            cerr<<"error when receiving data segment"<<endl;
            exit(EXIT_FAILURE);
        } 
        size_t new_pos = Is_Last_Chunk(temp);
        if(new_pos!=0){
            for(size_t i = 0; i < new_pos;i++){
                res.push_back(temp[i]);
            }
        }else{
            for(size_t i = 0; i < temp_size;i++){
                res.push_back(temp[i]);
            }
        }

        if(temp_size == 0){
            res.push_back('\0');
            return res;
        }
        fill(temp.begin(),temp.end(), '\0');
    }
}


        fd_set readfds;
        cout<<"CONNECT connection established, monitoring inputs and transeferring messages"<<endl;
        while(true) {
            //cout<<"waiting"<<endl;
            FD_ZERO(&readfds);
            FD_SET(server_fd,&readfds);
            FD_SET(client,&readfds);

            int select_status = select(sizeof(readfds)*3, &readfds, NULL, NULL, NULL);//change
            if(select_status == -1) {
                cerr<<"fail selecting in connect"<<endl;
                exit(EXIT_FAILURE);
            }

            if(FD_ISSET(server_fd, &readfds)) {
                vector<char> recv_data(40960);
                fill(recv_data.begin(), recv_data.end(), '\0');
        
                
                int recvdata_size = recv(server_fd, recv_data.data(), 40960, 0);
                if(recvdata_size < 0) {
                    cerr<<"fail to receive data from source"<<endl;
                    exit(EXIT_FAILURE);
                }
                if(recvdata_size == 0) {
                    cerr<<"source closed"<<endl;
                    return;
                }
                
                cout<<"received data size in CONNECT is"<<recvdata_size<<endl;
                recv_data.push_back('\0');
                
                int new_status = send(client_fd, recv_data.data(), recvdata_size, MSG_NOSIGNAL);
                if(new_status < 0) {
                    cerr<<"cannot transfer message to target"<<endl;
                    exit(EXIT_FAILURE);
                }
            }else if(FD_ISSET(client_fd, &readfds)){
                vector<char> recv_data(40960);
                fill(recv_data.begin(), recv_data.end(), '\0');
        
                
                int recvdata_size = recv(client_fd, recv_data.data(), 40960, 0);
                if(recvdata_size < 0) {
                    cerr<<"fail to receive data from source"<<endl;
                    exit(EXIT_FAILURE);
                }
                if(recvdata_size == 0) {
                    cerr<<"source closed"<<endl;
                    return;
                }
                
                cout<<"received data size in CONNECT is"<<recvdata_size<<endl;
            
                
                int new_status = send(server_fd, recv_data.data(), recvdata_size, MSG_NOSIGNAL);
                if(new_status < 0) {
                    cerr<<"cannot transfer message to target"<<endl;
                    exit(EXIT_FAILURE);
                }
            }
            
        }