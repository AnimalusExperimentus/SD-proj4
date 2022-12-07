#include "zookeeper/zookeeper.h"

#ifdef _ZOOKEEP_H
#define _ZOOKEEP_H

void start_conn(String localHost);

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);


#endif