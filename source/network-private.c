/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include <unistd.h>
#include <stdio.h>

int write_all(int sock, char *buf, int len) {
    int size = len;
    while (size > 0) {
        int res = (int) write(sock, buf, size);
        if (res < 0) {
            printf("Something went wrong - write all");
            return res;
        }
        buf += res;
        size -= res;
    }
    return len;
}


int read_all(int sock, char *buf, int len) {
    int size = len;
    while (size > 0) {
        int res = (int) read(sock, buf, size);
        if (res == 0)
            return 0;
        if (res < 0) {
            printf("Something went wrong - read all");
            return res;
        }
        buf += res;
        size -= res;
    }
    return len;
}