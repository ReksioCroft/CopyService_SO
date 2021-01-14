#ifndef COPYSERVICE_SO_CHANNEL_H
#define COPYSERVICE_SO_CHANNEL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define nMax 1024

typedef struct copyjob_T{
    char src[nMax];
    char dst[nMax];
    char task[10];
    int threadId;
} copyjob_t;

typedef struct copyjob_Stats {
    float progres;
    char state[32];
    int nrJobs;
    struct copyjob_Stats* nextJob;
    int threadId;
} copyjob_stats;

copyjob_stats* client( copyjob_t* );

copyjob_t* myDaemon( copyjob_stats* );


#endif //COPYSERVICE_SO_CHANNEL_H
