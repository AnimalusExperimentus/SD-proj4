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
#include "../include/client_stub.h"


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Server takes 1 argument: <Host:Port> of Zookeper\n");
        exit(-1);
    }

    char *host_port = argv[1];
    int port = zoo_conn(host_port);
    
    int listening_socket = network_server_init((short)port);
    if (listening_socket == -1) {
        printf("Socket creation error\n");
        exit(-1);
    }

    if (tree_skel_init(1) != 0) {
        printf("Tree initialization error\n");
        exit(-1);
    }

    int result = network_main_loop(listening_socket);
    
    tree_skel_destroy();
    printf("Exiting...\n");
    exit(result);
}