/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H


struct op_proc {
    // maior identificador de ops de escrita ja concluidas
    int max_proc;
    // ops de escrita a serem atendidas pelos threads neste momento
    int *in_progress;
};

struct request_t {
    int op_n;            //o número da operação
    int op;              //a operação a executar. op=0 se for um delete, op=1 se for um put
    char* key;           //a chave a remover ou adicionar
    struct data_t *data; // os dados a adicionar em caso de put, ou NULL em caso de delete
    struct request_t *next;
};


/*
Adiciona uma operação a ser feita a uma pilha.
Esta é acedida por uma da threads que executa a operação process_request() 
*/
void queue_add_request(struct request_t *request);

/*
Devolve a operação seguite da pilha eliminando essa da pilha atual
Tem uma condição caso não exita nada na pilha
*/
struct request_t *queue_get_request();



#endif