/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

typedef struct String_vector zoo_string; 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/network_client.h"
#include "../include/client_stub_private.h"
#include "../include/sdmessage.pb-c.h"
#include "../include/data.h"
#include "/usr/include/zookeeper/zookeeper.h"

#define ZDATALEN 1024 * 1024

static zhandle_t *c_zh;
static char *c_root_path = "/chain";
static int is_connected;

struct rtree_t *rtree_head = NULL;
struct rtree_t *rtree_tail = NULL;
// static char *c_watcher_ctx = "ZooKeeper Data Watcher";

//  zookeeper_close(zh);

/*
* Watcher function for connection state change events
*/
void conn_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

/*
* Client data watcher function for children of /chain
*/
// static void c_child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *c_watcher_ctx) {
// 	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
// 	// int zoo_data_len = ZDATALEN;
// 	if (state == ZOO_CONNECTED_STATE)	 {
// 		if (type == ZOO_CHILD_EVENT) {
// 	 	   /* Get the updated children and reset the watch */ 
//  			if (ZOK != zoo_wget_children(c_zh, c_root_path, c_child_watcher, c_watcher_ctx, children_list)) {
//  				fprintf(stderr, "Error setting watch at %s!\n", c_root_path);
//  			}
// 			fprintf(stderr, "\n=== znode listing === [ %s ]", c_root_path); 
// 			for (int i = 0; i < children_list->count; i++)  {
// 				fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
// 			}
// 			fprintf(stderr, "\n=== done ===\n");
// 		 } 
// 	 }
// 	 free(children_list);
// }


/*
* Connects to Zookeeper
* find and connect to head and tail of chain
*/
void client_zoo_conn(char *host_port) {

    // Connect to running Zookeeper Server
    zoo_set_debug_level((ZooLogLevel)0);
    c_zh = zookeeper_init(host_port, conn_watcher, 2000, 0, NULL, 0); 
    if (c_zh == NULL)	{
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Established connection to Zookeeper Server sucessfully\n");
    }
    sleep(1);

    // find head and tail servers of the chain
    zoo_string *children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	if (ZOK != zoo_get_children(c_zh, c_root_path, 0, children_list)) {exit(EXIT_FAILURE);}
    int max = 0;
    int head_i = 0;
    int tail_i = 0;
    int min = INT_MAX;
    for (int i = 0; i < children_list->count; i++) {
        char *temp = strdup(children_list->data[i]);
        memmove(temp, temp+4, strlen(temp));
        int n = atoi(temp);
        if (n > max) { max = n; head_i = i;}
        if (n < min) { min = n; tail_i = i;}
        free(temp);
    }

    // get head and tail host:port and connect to them
    char h_node_path[120] = ""; char t_node_path[120] = "";
    strcat(h_node_path,"/chain/"); 
    strcat(h_node_path, children_list->data[head_i]);
    strcat(t_node_path,"/chain/"); 
    strcat(t_node_path, children_list->data[tail_i]);
    char h_data[1024]; char t_data[1024];
    int h_len = 1024; int t_len = 1024;
    if (ZOK != zoo_get(c_zh, h_node_path, 0, h_data, &h_len, NULL)) {exit(EXIT_FAILURE);}
    if (ZOK != zoo_get(c_zh, t_node_path, 0, t_data, &t_len, NULL)) {exit(EXIT_FAILURE);}
    free(children_list);
    
    printf("Head: %s\n", h_data);
    printf("Tail: %s\n", t_data);

    rtree_head = rtree_connect(h_data);
    rtree_tail = rtree_connect(t_data);
    if (rtree_head == NULL || rtree_tail == NULL) {
        printf("Error connecting to chain\n");
        exit(EXIT_FAILURE);
    }
}


// -----------------------------------------------------------------------------------------------------

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port) {
    
    if(address_port == NULL){
        return NULL;
    }

    struct rtree_t *rTree;
    
    // Create and allocate rtree
    rTree = malloc(sizeof(struct rtree_t));
    if( (rTree == NULL)) {return NULL;} 
    
    // get address and port
    char copAdr [strlen(address_port)];
    strcpy(copAdr,address_port);
    char *adr = strtok(copAdr, ":");
    char* ptr;
    int port = (int) strtol( strtok(NULL,"\0"), &ptr, 10);

    // save socket data in rtree
    rTree->server.sin_family = AF_INET;
    rTree->server.sin_port = htons(port);
    rTree->server.sin_addr.s_addr = inet_addr(adr);

    // connect to server
    if ((network_connect(rTree)) == -1) {
        free(rTree);
        return NULL;
    }

    return rTree;
}


/* Termina a associação entre o cliente e os servidores, fechando as
 * ligaçoes e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect() {
    if (rtree_head == NULL || rtree_tail == NULL) {
        return -1;
    }
    if (network_close(rtree_head) == -1 || network_close(rtree_tail) == -1) {
        return -1;
    }
    free(rtree_head);
    free(rtree_tail);
    return 0;
}


//-------------------------------------------------------------------------------
/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct entry_t *entry, struct rtree_t *rtree) {

    // create and fill message with entry
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    msg.key = malloc(strlen(entry->key)+1);
    memcpy(msg.key, entry->key, strlen(entry->key)+1);
    msg.size = strlen(entry->key)+1;
    msg.data.len = entry->value->datasize;
    msg.data.data = malloc(entry->value->datasize);
    memcpy(msg.data.data, entry->value->data, entry->value->datasize);

    // send and receive response
    MessageT *msg_rcv = network_send_receive(rtree, &msg);
    free(msg.key);
    free(msg.data.data);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    

    int op_n = msg_rcv->op_n;
    message_t__free_unpacked(msg_rcv, NULL);
    return op_n;
}

int rtree_put_aux(struct entry_t *entry) {
    return rtree_put(entry, rtree_head);
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(char *key) {

    // create and fill message with entry
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    int len = strlen(key)+1;
    msg.key = malloc(len);
    memcpy(msg.key, key, len);
    msg.size = len;

    // send msg and receive response
    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    free(msg.key);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }
    
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }

    // create data struct to return from msg
    struct data_t *d;
    if (msg_rcv->data.data == NULL) {
        d = NULL;
    } else {
        d = data_create(msg_rcv->size);
        memcpy(d->data, msg_rcv->data.data, d->datasize);
    }

    message_t__free_unpacked(msg_rcv, NULL);
    return d;
}


/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(char *key, struct rtree_t *rtree) {

    // create message
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    int len = strlen(key)+1;
    msg.key = malloc(len);
    memcpy(msg.key, key, len);
    msg.size = len;

    // send msg and receive response
    MessageT *msg_rcv = network_send_receive(rtree, &msg);
    free(msg.key);

    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }

    int op_n = msg_rcv->op_n;
    message_t__free_unpacked(msg_rcv, NULL);
    return op_n;
}

int rtree_del_aux(char *key) {
    return rtree_del(key, rtree_head);
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size() {

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    
    int size = msg_rcv->size;

    message_t__free_unpacked(msg_rcv, NULL);
    return size;
}


/* Função que devolve a altura da árvore.
 */
int rtree_height() {

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -1;
    }
    
    int size = msg_rcv->size;

    message_t__free_unpacked(msg_rcv, NULL);
    return size;
}


/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys() {

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }

    // copy keys to local array to return
    int size = msg_rcv->n_keys;
    char **key_arr = malloc(sizeof(char *)*(size+1));
    key_arr[size] = NULL;
    
    for (int i = 0; i < size; i++) {
        int len = strlen(msg_rcv->keys[i]) + 1;
        key_arr[i] = malloc(len);
        memcpy(key_arr[i], msg_rcv->keys[i], len);
    }
    
    message_t__free_unpacked(msg_rcv, NULL);
    return key_arr;
}


/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values() {

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETVALUES;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }
    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return NULL;
    }

    // build result array
    int n_values = msg_rcv->n_vals;
    void  **result_arr = malloc(sizeof(struct data_t *)*(n_values+1));
    result_arr[n_values] = NULL;
    for (int i = 0; i < n_values; i++) {

        struct data_t *d = malloc(sizeof(struct data_t));
        d->datasize = msg_rcv->vals[i]->data.len;
        d->data = malloc((sizeof(uint8_t *))*d->datasize);
        memcpy(d->data, msg_rcv->vals[i]->data.data, d->datasize);
        result_arr[i] = d;
    }
    
    message_t__free_unpacked(msg_rcv, NULL);
    return result_arr;
}


/* Verifica se a operacao identificada por op_n foi executada
*/
int rtree_verify( int op_n) {

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.op_n = op_n;

    MessageT *msg_rcv = network_send_receive(rtree_tail, &msg);
    if (msg_rcv == NULL) {
        message_t__free_unpacked(msg_rcv, NULL);
        return  -1;
    }

    if (msg_rcv->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(msg_rcv, NULL);
        return -2;
    }

    int r = msg_rcv->op_n;
    message_t__free_unpacked(msg_rcv, NULL);
    return r;
}
