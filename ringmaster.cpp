#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"potato.h"
using namespace std;

vector<int> fd_vec;
vector<string> ip_vec;
vector<int> port_vec;

void connect_with_players(int socket_fd,int num_player,int hops){
    for(int i =0 ; i < num_player;i++){
        struct sockaddr_storage player_addr;
        socklen_t len_player_addr = sizeof(player_addr);
        int player_port;
        string player_ip;
        int player_fd = accept(socket_fd,(struct sockaddr*)&player_addr,&len_player_addr);
        if(player_fd == -1){
            cerr<<"Error: cannot accept connection on socket" << endl;
            exit(EXIT_FAILURE);
        }

        player_ip = inet_ntoa(((struct sockaddr_in *)&player_addr)->sin_addr); 
        send(player_fd,&i,sizeof(i),0);//send player's id
        send(player_fd,&num_player,sizeof(num_player),0);//send num of players
        recv(player_fd,&player_port,sizeof(player_port),MSG_WAITALL);
        fd_vec.push_back(player_fd);
        ip_vec.push_back(player_ip);
        port_vec.push_back(player_port);
        cout<<"Player "<<i<<" is ready to play"<<endl;
    }

    for (int i = 0; i < num_player; i++)
    {
        //select the server player for curr player
        int right_player_id = (i+1)% num_player;
        int right_player_port = port_vec[right_player_id];
        string right_player_addr = ip_vec[right_player_id];
        char right_player_addr_c[100];
        memset(right_player_addr_c,0,sizeof(right_player_addr_c));//？？？
        strcpy(right_player_addr_c, right_player_addr.c_str());//???
        //send the server player's addr and port to curr player
        //for the curr player to operate
        send(fd_vec[i],&right_player_port,sizeof(right_player_port),0);
        send(fd_vec[i],&right_player_addr_c,sizeof(right_player_addr_c),0);
    }

    Potato potato;
    potato.hops = hops;
    
    int random = getRandom(num_player);
    cout << "Ready to start the game, sending potato to player " << random << endl;
    send(fd_vec[random],&potato,sizeof(potato),0);
    fd_set readfds;
    int maxFd = fd_vec[0];
    FD_ZERO(&readfds);
    for(int i=0;i<num_player;i++){
        FD_SET(fd_vec[i],&readfds);
    }

    for (int i = 0; i < fd_vec.size(); i++)
    {
        if(fd_vec[i] > maxFd){
            maxFd = fd_vec[i];
        }
    }
    maxFd += 1;
    select(maxFd,&readfds,NULL,NULL,NULL);
    for (int i = 0; i < fd_vec.size(); i++)
    {
        if(FD_ISSET(fd_vec[i],&readfds)){////recv the potato 
            recv(fd_vec[i],&potato,sizeof(potato),MSG_WAITALL);
            break;
        }
    }

    
    for(int i = 0; i < fd_vec.size();i++){
        send(fd_vec[i],&potato,sizeof(potato),0);
    }

    if(hops != 0){
        std::cout << "Trace of potato:" << std::endl;
        for (int i = 0; i < potato.count-1; i++)
        {
            std::cout << potato.trace[i]<<",";
            //printf("%d",potato.trace[i]);
        }
        cout << potato.trace[potato.count-1] << endl;
        //printf("%d",potato.trace[potato.count-1]);
    }

    for (int i = 0; i < fd_vec.size(); i++)
    {
        close(fd_vec[i]);
    }
}

int main(int argc,char *argv[]){
    if(argc!=4){
        cout << "Wrong Format!" << endl;
        exit(EXIT_FAILURE);
    }

    int num_player = atoi(argv[2]);
    int hops = atoi(argv[3]);

    cout<< "Potato Ringmaster"<<endl;
    if(num_player <= 1){
        cerr << "num of players cannot less than 1"<<endl;
        exit(EXIT_FAILURE);
    }
    cout<< "Players = " <<num_player<< endl;
    if(hops < 0 || hops > 512){
        cerr << "num of hops cannot less than 0 or bigger than 512"<<endl;
        exit(EXIT_FAILURE);
    }
    cout << "Hops = " << hops << endl;

    //init the server on ringmaster
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port = argv[1];

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  ringmaster:status" << endl;
        return -1;
    } //if

    socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << " ringmaster:socket" << endl;
        return -1;
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    status = listen(socket_fd, 100);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if
    freeaddrinfo(host_info_list);
    connect_with_players(socket_fd,num_player,hops);
    close(socket_fd);
    exit(EXIT_SUCCESS);
}