#include "zookeeper/zookeeper.h"

#ifdef _ZOOKEEP_H
#define _ZOOKEEP_H

void start_conn(String localHost);

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

void disc_zoo();


#endif