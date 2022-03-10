#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <arpa/inet.h>
#include"potato.h"
using namespace std;

void Player::connectMaster(const char* hostname,const char* port){//establish connection with ringmaster
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << " connectMaster:status" << endl;
        exit(EXIT_FAILURE);
    } 

    master_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (master_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << " connectmaster:master fd" << endl;
        exit(EXIT_FAILURE);
    } //if
    
    status = connect(master_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    } //if

    // after connected, recv the player id and num_players
    recv(master_fd,&id,sizeof(id),MSG_WAITALL);
    recv(master_fd,&num_players,sizeof(num_players),MSG_WAITALL);
    cout << "Connected as player " << id << " out of " << num_players << " total players"<<endl;
    freeaddrinfo(host_info_list);
}

void Player::connetAndAccept(){
    //init a server and send the server's port num to the master
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char * port = "";//

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  connetAndAccept1" << endl;
        exit(EXIT_FAILURE);
    } //if


    server_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
    if (server_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    } //if

    int yes = 1;
    status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    } //if

    status = listen(server_fd, 100);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    } //if
    freeaddrinfo(host_info_list);
    /*#########################################################################################*/
    //get port???
    struct sockaddr_in addr;
    socklen_t len_addr = sizeof(addr);
    getsockname(server_fd,(struct sockaddr *)&addr,&len_addr);
    int server_port  = ntohs(addr.sin_port);//???
    send(master_fd,&server_port,sizeof(server_port),0);
    
    /*#########################################################################################*/
    //receive (player_id+1)'s port num and ip addr from master and connect to it as a client
    char my_right[100];
    int my_right_port;
    recv(master_fd,&my_right_port,sizeof(my_right_port),MSG_WAITALL);
    recv(master_fd,&my_right,sizeof(my_right),MSG_WAITALL);
    char format_my_right_port[9];
    sprintf(format_my_right_port, "%d",my_right_port);//疑惑
    //become the client of the neighbour
    int status2;
    struct addrinfo host_info2;
    struct addrinfo *host_info_list2;
    const char *hostname2 = my_right;
    const char *port2 = format_my_right_port;
    memset(&host_info2, 0, sizeof(host_info2));
    host_info2.ai_family   = AF_UNSPEC;
    host_info2.ai_socktype = SOCK_STREAM;

    status2 = getaddrinfo(hostname2, port2, &host_info2, &host_info_list2);
    if (status2 != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  connetAndAccept:status2" << endl;
        exit(EXIT_FAILURE);
    } //if

    right_fd = socket(host_info_list2->ai_family, 
                host_info_list2->ai_socktype, 
                host_info_list2->ai_protocol);
    if (right_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname2 << "," << port2 << ")" << endl;
        exit(EXIT_FAILURE);
    } //if

    status2 = connect(right_fd, host_info_list2->ai_addr, host_info_list2->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname2 << "," << port2 << ")" << endl;
        exit(EXIT_FAILURE);
    } //if

    freeaddrinfo(host_info_list2);
    right_id = (id+1)%num_players;//111
    /*#########################################################################################*/

    //act as a server and accept for (player_id-1) to connect
    string left_addr;
    struct sockaddr_storage my_left_addr;
    socklen_t len_my_left_addr = sizeof(my_left_addr);
    left_fd = accept(server_fd,(struct sockaddr *)&my_left_addr,&len_my_left_addr);
    if (left_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        exit(EXIT_FAILURE);
    } //if
    left_addr = inet_ntoa(((struct sockaddr_in *)&my_left_addr)->sin_addr);
    left_id = (id-1+num_players)%num_players;
    
}

void Player::assignPotato(){
    Potato potato;
    int fd[3] = {master_fd,left_fd,right_fd};

    while (true)
    {
        fd_set readfds;
        int maxFd = fd[0];
        FD_ZERO(&readfds);
        FD_SET(fd[0],&readfds);
        FD_SET(fd[1],&readfds);
        FD_SET(fd[2],&readfds);
        for (int i = 0; i < 3; i++)
        {
            if(fd[i] > maxFd){
                maxFd = fd[i];
            }
        }
        maxFd += 1;
        select(maxFd,&readfds,NULL,NULL,NULL);
        for (int i = 0; i < 3; i++)
        {
            if(FD_ISSET(fd[i],&readfds)){////recv the potato ??
                recv(fd[i],&potato,sizeof(potato),MSG_WAITALL);
                break;
            }
        }
        if(potato.hops != 0){
            potato.hops--;
            potato.trace[potato.count] = id;
            potato.count++;

            if(potato.hops <= 0){
                cout << "I'm it" << endl; 
                send(master_fd,&potato,sizeof(potato),0);
            }else if(potato.hops > 0){
                /*srand((unsigned int)time(NULL)+potato.count);
                int randNeigh = rand() %2 ;*/
                int randNeigh = getRandom(potato.count);
                if(randNeigh == 0){
                    cout<< "Sending potato to " << right_id << endl;
                    send(right_fd,&potato,sizeof(potato),0);
                }else{
                    cout<< "Sending potato to " << left_id << endl;
                    send(left_fd,&potato,sizeof(potato),0);
                }
            }   
        }else{//potato.hops =0
            //cout << "Num of hops is 0, ringmaster has killed the game" <<endl;
            break;
            //exit(EXIT_FAILURE);
        }        
    }
}


int main(int argc,char *argv[]){
    if(argc != 3){
        cout << "invalid input!" << endl;
        return -1;
    }

    Player *player = new Player();
    player->connectMaster(argv[1],argv[2]);
    player->connetAndAccept();
    player->assignPotato();
    exit(EXIT_SUCCESS);
}