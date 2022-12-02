/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include <netinet/in.h>

/* Remote tree.
 */
struct rtree_t {  
    struct sockaddr_in server;
    int sockfd;
};

#endif //_CLIENT_STUB_PRIVATE_H