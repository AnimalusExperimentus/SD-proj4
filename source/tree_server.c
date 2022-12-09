/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/network_server.h"
#include "../include/zookeep.h"
#include "../include/client_stub.h"


int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Server takes 2 arguments:ip port\n");
        exit(-1);
    }

    char* ip;
    if(sscanf(argv[1], "%s", &ip) != 1) {
        printf("Port must be an integer\n");
        exit(-1);
    }
    int port;
    if(sscanf(argv[2], "%i", &port) != 1) {
        printf("Port must be an integer\n");
        exit(-1);
    }

    int listening_socket = network_server_init((short)port);
    if (listening_socket == -1) {
        printf("Socket creation error\n");
        exit(-1);
    }
    char *portC;
    sprintf(portC, "%d", port);
    char *adr=strcat(strcat(ip,":"),portC);
    start_conn(adr);

    if (tree_skel_init(1) != 0) {
        printf("Tree initialization error\n");
        exit(-1);
    }

    int result = network_main_loop(listening_socket);
    
    tree_skel_destroy();
    printf("Exiting...\n");
    exit(result);
}