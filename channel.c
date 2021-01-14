#include "channel.h"

copyjob_stats* client( copyjob_t* job ) {
    int channel;

    if ( job != NULL ) {
        char* channelName = "./clientFIFO";
        mkfifo( channelName, 0666 );

        channel = open( channelName, O_WRONLY );
        write( channel, job, sizeof( copyjob_t ) );
        close( channel );

        free( job );
        return NULL;
    }
    else {
        copyjob_stats* myJob = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
        char* channelName = "./daemonFIFO";
        mkfifo( channelName, 0666 );

        channel = open( channelName, O_RDONLY );
        read( channel, myJob, sizeof( copyjob_stats ) );
        close( channel );

        return myJob;
    }
}

copyjob_t* myDaemon( copyjob_stats* job ) {
    int channel;

    if ( job != NULL ) {
        char* channelName = "./daemonFIFO";
        mkfifo( channelName, 0666 );

        channel = open( channelName, O_WRONLY );
        write( channel, job, sizeof( copyjob_stats ) );
        close( channel );

        free( job );
        return NULL;
    }
    else {
        copyjob_t* myJob = ( copyjob_t* ) calloc( 1, sizeof( copyjob_t ) );

        char* channelName = "./clientFIFO";
        mkfifo( channelName, 0666 );

        channel = open( channelName, O_RDONLY );
        read( channel, myJob, sizeof( copyjob_t ) );
        close( channel );

        return myJob;
    }
}