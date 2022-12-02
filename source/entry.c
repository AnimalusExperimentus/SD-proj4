/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include "../include/data.h"
#include "../include/entry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Função que cria uma entry, reservando a memória necessária para a
 * estrutura e inicializando os campos key e value, respetivamente, com a
 * string e o bloco de dados passados como parâmetros, sem reservar
 * memória para estes campos.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {
    
    if(key==NULL&&data==NULL){
        return NULL;
    }
    
    struct entry_t *entry;
    entry = malloc(sizeof(struct entry_t));
    
    entry->key = key;
    entry->value = data;
    
    return entry;
}


/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry) {

    if(entry != NULL){
        data_destroy(entry->value);
        free(entry);
    }
}


/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry) {


    struct entry_t *ptr;

    ptr = malloc(sizeof(struct entry_t));
    ptr->key=malloc(strlen(entry->key));
    ptr->key=strcpy(ptr->key,entry->key);

    ptr->value=data_dup(entry->value);

    return ptr;
}


/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {
    data_destroy(entry->value);
    free(entry->key);

    entry->value=new_value;
    entry->key=new_key;
}


/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2) {

    return strcmp(entry1->key, entry2->key);
}
