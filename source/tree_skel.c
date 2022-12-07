/*
*   Grupo 12
*   Duarte Lopes Pinheiro nº54475
*   Filipe Henriques nº55228
*   Márcio Moreira nº41972
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "zookeeper/zookeeper.h"
#include "../include/sdmessage.pb-c.h"
#include "../include/tree.h"
#include "../include/tree_skel.h"
#include "../include/tree_skel_private.h"
#include "../include/data.h"
#include "../include/entry.h"
#include "../include/zookeep.h"


struct op_proc *proc_op;
struct request_t *queue_head;

struct tree_t *tree;
int last_assigned = 1;
int keep_t_running = 1;

pthread_t *threads;
int *thread_params;
int thread_num;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t tree_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t op_proc_lock = PTHREAD_MUTEX_INITIALIZER;

static zhandle_t *zh;


void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}


void start_conn(locaHost){
    zh = zookeeper_init(localHost, connection_watcher,	2000, 0, NULL, 0); 
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}
    sleep(3);
}

/**/
void queue_add_request(struct request_t *request) {

    pthread_mutex_lock(&queue_lock);
    request->next = NULL;
    if(queue_head == NULL) {
        // adds to head of FIFO
        queue_head = request;
    } else { 
        // adds to end of FIFO
        struct request_t *tptr = queue_head;
        while(tptr->next != NULL) {tptr = tptr->next;}
        tptr->next = request;
    }
    pthread_cond_broadcast(&queue_not_empty);
    pthread_mutex_unlock(&queue_lock);
}


/**/
struct request_t *queue_get_request() {

    pthread_mutex_lock(&queue_lock);
    while (queue_head == NULL && keep_t_running) {
        pthread_cond_wait(&queue_not_empty, &queue_lock);
    }
    if (!keep_t_running) { // ctrl^c
        pthread_mutex_unlock(&queue_lock);
        return NULL;
    }
    struct request_t *request = queue_head;
    queue_head = request->next;
    pthread_mutex_unlock(&queue_lock);
    
    return request;
}


/* Funcao da thread secundaria que vai processar pedidos de escrita
*/
void *process_request (void *params) {

    // pthread_detach(pthread_self());
    int thread_n = *((int*)params);

    struct request_t *request;
    while (true) {
        
        request = queue_get_request();
        if (request == NULL) { break; } // ctrl^c
        int op_n = request->op_n;
        // different threads don't touch/check eachother's
        // in_progress value, no lock needed
        proc_op->in_progress[thread_n] = op_n;

        // update tree
        pthread_mutex_lock(&tree_lock);
        if (request->op == 0) {
            tree_del(tree, request->key);
        } else {
            tree_put(tree, request->key, request->data);
        }
        pthread_mutex_unlock(&tree_lock);

        // update proc_op
        proc_op->in_progress[thread_n] = 0;
        pthread_mutex_lock(&op_proc_lock);
        if (proc_op->max_proc < op_n) {
            proc_op->max_proc = op_n;
        }
        pthread_mutex_unlock(&op_proc_lock);

        // free request
        free(request->key);
        if (request->op == 1)
            data_destroy(request->data);
        free(request);
    }

    // Signal next thread to exit
    pthread_cond_signal(&queue_not_empty);
    return NULL;
}


/* Verifica se a operacao identificada por op_n foi executada.
*/
int verify(int op_n) {

    // the main thread can read at any time without
    // compromising the integrity, worse that can happen
    // is client gets outdated answer on the status
    // but on resending will get correct one
    if (op_n <= proc_op->max_proc) { return 0; } 
    for (int i = 0; i < thread_num; i++)
    {
        if (proc_op->in_progress[i] == op_n) { 
            return 1; 
        }
    }
    return -2;       
}


/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * A funcao deve lancar N threads secundarias responsaveis por atender
 * pedidos de escrita na arvore;
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init(int N) {

    thread_num = N;
    
    tree = tree_create();
    if (tree == NULL) { return -1; }

    queue_head = NULL;
    
    // create and initialize proc_op
    proc_op = malloc(sizeof(struct op_proc));
    proc_op->in_progress = malloc((sizeof(int))*N);
    if (proc_op == NULL) {
        free(proc_op);
        return(-1);
    }
    proc_op->max_proc = 0;
    for(int i = 0; i < N; i++) { proc_op->in_progress[i] = 0; }

    // init threads
    threads = malloc(sizeof(pthread_t)*N);
    thread_params = malloc(sizeof(int)*N);

    // create threads
    for (int i = 0; i < N; i++){
		thread_params[i] = i;
		if (pthread_create(&threads[i], NULL, &process_request, (void *) &thread_params[i]) != 0) {
			printf("Erro na criacao da thread %d.\n", i);
			return(-1);
		}
	}

    return 0;
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy() {
    
    // wait until all requests have been processed
    while (queue_head != NULL) { sleep(1); }
    // we can join threads safely now
    keep_t_running = 0;
    pthread_cond_signal(&queue_not_empty);
    for (int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("\nClosed all threads\n");
    free(threads); free(thread_params);
    // free tree
    if(tree != NULL) { tree_destroy(tree); }
    //free proc_op
    free(proc_op->in_progress); free(proc_op);

    // free request queue
    struct request_t *req;
    if(queue_head != NULL) {
        while(queue_head->next != NULL) {
            req=queue_head->next;
            free(queue_head->key);
            data_destroy(queue_head->data);
            free(queue_head);
            queue_head=req;
        }
        free(queue_head);
    }
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(MessageT *msg) {
    
    if(msg==NULL || msg->opcode < 0 || msg->c_type < 0 || msg->opcode > MESSAGE_T__OPCODE__OP_ERROR || msg->c_type > MESSAGE_T__C_TYPE__CT_NONE) {
        return -1;
    }
    switch(msg->opcode) {
        case MESSAGE_T__OPCODE__OP_SIZE:
        {
            msg->opcode=MESSAGE_T__OPCODE__OP_SIZE+1;
            msg->c_type=MESSAGE_T__C_TYPE__CT_RESULT;
            pthread_mutex_lock(&tree_lock);
            msg->size=tree_size(tree);
            pthread_mutex_unlock(&tree_lock);
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_HEIGHT:
        {
            msg->opcode=MESSAGE_T__OPCODE__OP_HEIGHT+1;
            msg->c_type=MESSAGE_T__C_TYPE__CT_RESULT;
            pthread_mutex_lock(&tree_lock);
            msg->size=tree_height(tree);
            pthread_mutex_unlock(&tree_lock);
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_DEL:
        {
            int opnumber = last_assigned;
            last_assigned++;
            
            char* key_d = malloc(msg->size);
            memset(key_d, '\0', msg->size);
            memcpy(key_d, msg->key, msg->size);

            struct request_t *req;
            req = malloc(sizeof(struct request_t));
            req->op_n = opnumber;
            req->op = 0;
            req->key = key_d;

            queue_add_request(req);
            msg->opcode=MESSAGE_T__OPCODE__OP_DEL+1;
            msg->c_type=MESSAGE_T__C_TYPE__CT_RESULT;
            msg->op_n=opnumber;
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_GET:
        {
            char *key = malloc(msg->size);
            memset(key, '\0', msg->size);
            memcpy(key, msg->key, msg->size);

            pthread_mutex_lock(&tree_lock);
            struct data_t *t = tree_get(tree, key);
            pthread_mutex_unlock(&tree_lock);

            if(t == NULL) {
                msg->opcode=MESSAGE_T__OPCODE__OP_GET+1;
                msg->c_type=MESSAGE_T__C_TYPE__CT_VALUE;
                msg->data.data = NULL;
                msg->data.len = 0;
                msg->size = 0;
            } else {
                msg->opcode=MESSAGE_T__OPCODE__OP_GET+1;
                msg->c_type=MESSAGE_T__C_TYPE__CT_VALUE;
                
                msg->data.data = malloc(t->datasize);
                msg->data.len = t->datasize;
                msg->size = t->datasize;
                memcpy(msg->data.data, t->data, msg->size);
            }
            free(key);
            data_destroy(t);
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_PUT:
        {
            int opnumber = last_assigned;
            last_assigned++;

            struct data_t *new_data = data_create((int)msg->data.len);
            memcpy(new_data->data, msg->data.data, msg->data.len);
            char* temp_key = malloc(msg->size);
            memcpy(temp_key, msg->key, msg->size);
            
            struct request_t *req;
            req = malloc(sizeof(struct request_t));
            req->op_n = opnumber;
            req->op = 1;
            req->key = temp_key;
            req->data = new_data;

            queue_add_request(req);
            msg->opcode=MESSAGE_T__OPCODE__OP_PUT+1;
            msg->c_type=MESSAGE_T__C_TYPE__CT_RESULT;
            msg->op_n=opnumber;
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_GETKEYS:
        {
            pthread_mutex_lock(&tree_lock);
            char** kk = tree_get_keys(tree);
            pthread_mutex_unlock(&tree_lock);

            if(kk != NULL){
                msg->opcode=MESSAGE_T__OPCODE__OP_GETKEYS+1;
                msg->c_type=MESSAGE_T__C_TYPE__CT_KEYS;

                int size = 0;
                for (int i = 0; kk[i] != NULL; i++) {
                    size++;
                }
                
                msg->n_keys = size;
                msg->keys = kk;

            }else{ 
                msg->opcode=MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type=MESSAGE_T__C_TYPE__CT_NONE;
            }
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_GETVALUES:
        {
            pthread_mutex_lock(&tree_lock);
            void **val = tree_get_values(tree);
            pthread_mutex_unlock(&tree_lock);

            if (val != NULL) {
                msg->opcode=MESSAGE_T__OPCODE__OP_GETVALUES+1;
                msg->c_type=MESSAGE_T__C_TYPE__CT_VALUES;

                
                int size = 0;
                for (int i = 0; val[i] != NULL; i++) {
                    size++;
                }

                msg->n_vals = size;
                msg->vals = malloc(sizeof(MessageT__Value *)*size);
                
                for (int i = 0; i < size; i++)
                {
                    struct data_t *d = val[i];

                    MessageT__Value *v;
                    v = malloc(sizeof(MessageT__Value));
                    message_t__value__init(v);
                    v->data.len = d->datasize;
                    v->data.data = malloc(d->datasize);
                    v->data.data = memcpy(v->data.data, d->data, d->datasize);
                    msg->vals[i] = v;
                    data_destroy(d);
                }
                free(val);

            } else {
                msg->opcode=MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type=MESSAGE_T__C_TYPE__CT_NONE;
            }
            return 0;
        }
        case MESSAGE_T__OPCODE__OP_VERIFY:
        {
            int r = verify(msg->op_n);
            if (r == 0 || r == 1) {
                msg->opcode=MESSAGE_T__OPCODE__OP_PUT+1;
                msg->c_type=MESSAGE_T__C_TYPE__CT_RESULT;
                msg->op_n=r;
            } else if (r == -2) {
                msg->opcode=MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type=MESSAGE_T__C_TYPE__CT_NONE;
                msg->op_n=r;
            }
            return 0;
        }
        // so compiler doesn't scream at us
        case MESSAGE_T__OPCODE__OP_BAD: { return 0; }
        case MESSAGE_T__OPCODE__OP_ERROR: { return 0; }
        case _MESSAGE_T__OPCODE_IS_INT_SIZE: { return 0; }
    }
    return -1;
}
