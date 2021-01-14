#ifndef COPYSERVICE_SO_CHANNEL_H
#define COPYSERVICE_SO_CHANNEL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct copyjob_T{
    char src[1024];
    char dst[1024];
    char task[50];
    int threadId;
} copyjob_t;

typedef struct copyjob_Stats {
    float progres;
    char state[32];
    struct copyjob_Stats* nextJob;
    int threadId;
} copyjob_stats;

copyjob_stats* client( copyjob_t* );

copyjob_t* myDaemon( copyjob_stats* );


#endif //COPYSERVICE_SO_CHANNEL_H
