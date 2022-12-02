/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <poll.h>
#include <signal.h>
#include "../include/tree_skel.h"
#include "../include/network-private.h"

#define SIZE_CLIENT_DEFAULT 1024
#define NFDESC 8 // accept atmost NFDESC clients at the same time
#define TIMEOUT 50 

int sckt;
static volatile int keep_running = 1;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port){
    if (port < 0) {
        perror("Port value is invalid");
        return -1;
    }

    int socket_fd;
    struct sockaddr_in server;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Something went wrong creating socket");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    const int enable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("Something went wrong - SO_REUSEADDR");
        close(socket_fd);
        return -1;
    }
    
    if (bind(socket_fd, (struct sockaddr *) &server, sizeof(server)) != 0) {
        perror("Something went wrong - bind server");
        close(socket_fd);
        return -1;
    }
    
    if (listen(socket_fd, 5) != 0) {
        perror("Something went wrong - listen server");
        close(socket_fd);
        return -1;
    }

    sckt = socket_fd;
    return socket_fd;
}


/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT *network_receive(int client_socket) {

    if(client_socket < 0) {
        perror("Invalid Socket");
        return NULL;
    }

    // read length of message and allocate buffer
    unsigned len;
    int r = read(client_socket, &len, sizeof(unsigned));
    if (r == 0) {
        printf("Client closed connection!\n");
        return NULL;
    }
    char *buf = malloc(len);

    // read message into buffer
    if ((read_all(client_socket, buf, (int) len)) < 0) {
        close(client_socket);
        return NULL;
    }

    MessageT *msg;
    msg = message_t__unpack(NULL, len, (const uint8_t *) buf);
    free(buf);
    if (msg == NULL) {
        perror("Error unpacking message\n");
        return NULL;
    }

    return msg;
}


/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg) {
    
    if (client_socket < 0 || msg == NULL) {
        perror("Client socket or message tried to send is invalid");
        return -1;
    }

    // allocate buffer for packed message and pack it
    unsigned len = message_t__get_packed_size(msg);
    void *buf = malloc(len);
    message_t__pack(msg, buf);
    message_t__free_unpacked(msg, NULL);

    // send message size first
    write(client_socket, &len, sizeof(unsigned));
    // send message
    if (write_all(client_socket, (char *) buf, (int) len) < 0) {
        free(buf);
        close(client_socket);
        return -1;
    }

    free(buf);
    return 0;
}


/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close() {
    if (close(sckt) == 0) {
        return 0;
    }
    return -1;
}


/* Aux function*/
int get_nfds(struct pollfd *connections) {

    int nfds = 1;
    for (nfds = 1; nfds < NFDESC; nfds++) {
        if (connections[nfds].fd == -1) {
            return nfds;
        }
    }
    return -1;
}

/* Aux function*/
void int_handler(int dummy) {
    keep_running = 0;
}


/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket) {

    signal(SIGINT, int_handler);
    struct sockaddr client;
    socklen_t size_client = SIZE_CLIENT_DEFAULT;
    struct pollfd connections[NFDESC];
    int nfds, kfds;
    
    if (listening_socket < 0) {
        perror("Listening socket is invalid\n");
        return -1;
    }

    // make poll ignore other sockets
    for (int i = 1; i < NFDESC; i++) { 
        connections[i].fd = -1;
        connections[i].events = 0;
    }

    // init welcoming socket for poll
    connections[0].fd = listening_socket;
    connections[0].events = POLLIN;

    // first connection at index 1
    nfds = 1;

    // wait for data on open sockets
    while ( (kfds = poll(connections, NFDESC, TIMEOUT)) >= 0 && keep_running) {
    if (kfds > 0) {

        // Do we have a new connection request
        if ((connections[0].revents & POLLIN) && (nfds != -1)) { 
           if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0) {
                // exchange hello packet
                char buf[1] = "1";
                read(connections[nfds].fd, buf, 1);
                write(connections[nfds].fd, buf, 1);
                
                // set bitflag on connection for data
                connections[nfds].events = POLLIN; 
                printf("Client connected!\n");
                
                nfds = get_nfds(connections);
            } else {
                perror("Error accepting client connection\n");
                return -1;
            }
        // refuse connection if max nr clients reached
        } else if ((connections[0].revents & POLLIN) && (nfds == -1)) {
            close(accept(connections[0].fd, (struct sockaddr *) &client, &size_client));
        }
        // Check the remaining sockets
        for (int i = 1; i < NFDESC; i++) {
            if (connections[i].fd == -1) {continue;}
            if (connections[i].revents & POLLIN) {

                MessageT *messageT = network_receive(connections[i].fd);
                if (messageT == NULL) {
                    close(connections[i].fd);
                    connections[i].fd = -1;
                } else {
                    // execute request and send response
                    invoke(messageT);
                    network_send(connections[i].fd, messageT);
                }
            }
            if (connections[i].revents & POLLHUP) {
                close(connections[i].fd);
                connections[i].fd = -1;
            }
        }
        // update nfds if max nr clients reached
        if (nfds == -1) { nfds = get_nfds(connections); }
    }}

    // close all sockets
    for (int i = 0; i < NFDESC; i++) {
        if (connections[i].fd != -1) { close(connections[i].fd); }
    }
    
    return 0;
}