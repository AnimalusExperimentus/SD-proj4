/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include "../include/client_stub.h"
#include "../include/entry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFERLEN 4096
struct rtree_t *rtree;

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Client takes 1 argument: <Server>:<Port>\n");
        exit(-1);
    }

    const char *addr = strtok(argv[1], " \n");  
    
    // Establish connection with server
    rtree = rtree_connect(addr);
    if (rtree == NULL) {
        perror("Error connecting to server\n");
        exit(-1);
    }
    printf("Connected to server!\n");
    printf("> ");

    char line[BUFFERLEN];
    while(fgets(line, BUFFERLEN, stdin)) {

        char* command = strtok(line, " \n");
        printf("\n");
        if (strcmp(command, "quit") == 0) {
            rtree_disconnect(rtree);
            break;
        // PUT -----------------------------------------------------
        } else if (strcmp(command, "put") == 0) {
            char* key = strtok(NULL, " ");
            char* data = strtok(NULL, "\n");
            if (key == NULL || data == NULL || strtok(NULL, " ") != NULL){
                printf("syntax: put <key> <data>\n");
                printf("\n> ");
                continue;
            }

            struct data_t *d = data_create(strlen(data)+1);
            memcpy(d->data, data, strlen(data)+1);
            struct entry_t *e = entry_create(key, d);
            int r = rtree_put(rtree, e);
            entry_destroy(e);

            if (r > 0) { 
                printf("Created request on server with number: %i\n", r); 
            } else {
                printf("Error on creating server request\n"); 
            }

        // GET -----------------------------------------------------
        } else if (strcmp(command, "get") == 0) {
            char* key = strtok(NULL, " \n");
            if (key == NULL || strtok(NULL, " ") != NULL){
                printf("syntax: get <key>\n");
                printf("\n> ");
                continue;
            }

            struct data_t *d = rtree_get(rtree, key);
            if (d == NULL) { 
                printf("Error in get\n");
                printf("\n> ");
                continue;
            }
            if (d->datasize == 0 ){
                printf("Could not find key in tree\n");
            } else {
                printf("Value found for this key: %.*s\n", d->datasize, (char*)d->data);
            }

        // DEL -----------------------------------------------------
        } else if (strcmp(command, "del") == 0) {
            char* key = strtok(NULL, " \n");
            if (key == NULL || strtok(NULL, " ") != NULL){
                printf("syntax: del <key>\n");
                printf("\n> ");
                continue;
            }
            
            int r = rtree_del(rtree, key);
            if(r > 0) {
                printf("Created request on server with number: %i\n", r);
            }else {
              printf("Error on creating server request\n");
            }

        // SIZE ---------------------------------------------------
        } else if (strcmp(command, "size") == 0) {
            if (strtok(NULL, " ") != NULL){
                printf("syntax: size\n");
                printf("\n> ");
                continue;
            }

            int r = rtree_size(rtree);
            if (r == -1) {
                perror("Error on size\n");
                exit(-1);
            }

            printf("Tree size is currently %i\n", r);

        // HEIGHT -----------------------------------------------
        } else if (strcmp(command, "height") == 0) {
            if (strtok(NULL, " ") != NULL){
                printf("syntax: height\n");
                printf("\n> ");
                continue;
            }

            int r = rtree_height(rtree);
            if (r == -1) {
                perror("Error on height\n");
                exit(-1);
            }

            printf("Tree height is currently %i\n", r);

        // GETKEYS ----------------------------------------------------
        } else if (strcmp(command, "getkeys") == 0) {
            if (strtok(NULL, " ") != NULL){
                printf("syntax: getkeys \n");
                printf("\n> ");
                continue;
            }

            char** keys = rtree_get_keys(rtree);
            
            if(keys==NULL){
                printf("Error on getkeys\n");
            } else {
                for (int i = 0; keys[i] != NULL; i++) {
                    printf("%s\n", keys[i]);
                }
            }

            // free keys
            for (int i = 0; keys[i] != NULL; i++)
                free(keys[i]);
            free(keys);

        // GETVALUES -----------------------------------------------
        } else if (strcmp(command, "getvalues") == 0) {
            if (strtok(NULL, " ") != NULL) {
                printf("syntax: getvalues <key>\n");
                printf("\n> ");
                continue;
            }

            void **values = rtree_get_values(rtree);
            if (values == NULL) {
                printf("Error on getvalues\n");
            } else {
                for (int i = 0; values[i] != NULL; i++) {
                    struct data_t *d = values[i];
                    int n = d->datasize;
                    printf("%.*s\n", n, (char *)d->data);
                }
            }

            // free values
            for (int i = 0; values[i] != NULL; i++) {
                struct data_t *d = values[i];
                free(d->data);
                free(d);
            }
            free(values);
        // VERIFY -----------------------------------------------
        } else if (strcmp(command, "verify") == 0) {
            char* op_n = strtok(NULL, " \n");
            if (op_n == NULL || strtok(NULL, " ") != NULL) {
                printf("syntax: verify <op_n>\n");
                printf("\n> ");
                continue;
            }

            int i_op_n;
            sscanf(op_n, "%d", &i_op_n);
            int r = rtree_verify(rtree, i_op_n);

            if (r == -1) {
                perror("Error receiving message\n");
                exit(-1);
            } else if (r == 1) {
                printf("Request is currently being processed\n");
            } else if ( r == 0) {
                printf("Request has been processed sucessfully!\n");
            } else if (r == -2) {
                printf("Request is still in the queue or doesn't exist\n");
            }
        } else {
            printf("That command doesn't exist.\n");
        }
        printf("\n> ");
    }
    exit(0);
}
