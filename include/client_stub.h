/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#ifndef _CLIENT_STUB_H
#define _CLIENT_STUB_H

#include "data.h"
#include "entry.h"

/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t;

/*
* Connects to Zookeeper
* finds and connects to head and tail servers of chain
*/
void client_zoo_conn(char *host_port);

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port);

/* Termina a associação entre o cliente e os servidores, fechando as
 * ligaçoes e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect();

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct entry_t *entry, struct rtree_t *rtree);
int rtree_put_aux(struct entry_t *entry);

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(char *key);

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(char *key, struct rtree_t *rtree);
int rtree_del_aux(char *key);

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size();

/* Função que devolve a altura da árvore.
 */
int rtree_height();

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys();

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values();

/*  Verifica se a operação op_n foi executada
 */
int rtree_verify(int op_n);


#endif
