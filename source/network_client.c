/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include "../include/client_stub.h"
#include "../include/sdmessage.pb-c.h"
#include "../include/client_stub_private.h"
#include "../include/network-private.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree) {

    if (rtree == NULL) {
        return -1;
    }

    // create socket and save in rtree
    rtree->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rtree->sockfd < 0) {
        perror("Error - socket creation\n");
        return -1;
    }

    // open connection to server
    if (connect(rtree->sockfd, (const struct sockaddr *) &rtree->server, sizeof(rtree->server)) < 0) {
        perror("Attempt to connect to server unsuccessful\n");
        close(rtree->sockfd);
        return -1;
    }

    // exchange hello packet
    char buf[1] = "1";
    write(rtree->sockfd, buf, 1);
    int r = read(rtree->sockfd, buf, 1);

    if (r <= 0) {
        perror("Server has reached the maximum number of clients connected\n");
        close(rtree->sockfd);
        return -1;
    }

    return 0;
}


/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtree_t * rtree, MessageT *msg) {

    if (rtree == NULL || msg == NULL) {
        perror("Client socket or message tried to send is invalid");
        return NULL;
    }
    
    unsigned len = message_t__get_packed_size(msg);

    void *buf = malloc(len);
    // Serialization using protobuf
    message_t__pack(msg, buf);

    // send size of message to send
    write(rtree->sockfd,&len,sizeof(unsigned));

    // send packed message
    if (write_all(rtree->sockfd, (char *) buf, (int) len) < 0) {
        close(rtree->sockfd);
        return NULL;
    }
    free(buf);


    // get the size of the message to receive
    unsigned len_rcv;
    read(rtree->sockfd, &len_rcv, sizeof(unsigned));
    
    // allocate buf for message to receive
    char *buf_rcv = malloc(len_rcv);

    // read message into buf_rcv
    if ((read_all(rtree->sockfd, (char *) buf_rcv, (int) len_rcv)) != len_rcv) {
        free(buf_rcv);
        close(rtree->sockfd);
        return NULL;
    }

    MessageT *messageT_deserialized = NULL;
    messageT_deserialized = message_t__unpack(NULL, len_rcv, (const uint8_t *) buf_rcv);

    free(buf_rcv);
    return messageT_deserialized;
}
                                       

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t * rtree){
     if (close(rtree->sockfd) == -1) {
        return -1;
    }
    return 0;
}

