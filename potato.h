#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>

class Potato{
public:
    int hops;
    int count;
    int trace[512];
    Potato():hops(0),count(0){
        memset(&trace,0,sizeof(trace));
    }
};

class Player{
public:
    int id;
    int num_players;
    int left_fd;
    int right_fd;
    int master_fd;
    int server_fd;
    int left_id;
    int right_id;
  

    Player():id(0),num_players(0),left_fd(-1),right_fd(-1),master_fd(-1),server_fd(-1),left_id(0),right_id(0){}
    ~Player(){
        close(master_fd);
        close(left_fd);
        close(right_fd);
        close(server_fd);
    }

    void connectMaster(const char *hostname,const char *port);
    void connetAndAccept();
    void assignPotato();
};

int getRandom(int num){
    srand((unsigned int)time(NULL)+num);
    return rand() %2 ;
}