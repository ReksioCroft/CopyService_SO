#include "channel.h"

copyjob_stats* client( copyjob_t* job ) {
    int pipe;

    if ( job != NULL ) {
        char* pipeName = "/tmp/clientToDaemonFIFO";
        mkfifo( pipeName, 0666 );

        pipe = open( pipeName, O_WRONLY );
        write( pipe, job, sizeof( copyjob_t ) );
        close( pipe );

        free( job );
        return NULL;
    }
    else {
        copyjob_stats* msg = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
        char* pipeName = "/tmp/daemonToClientFIFO";
        mkfifo( pipeName, 0666 );

        pipe = open( pipeName, O_RDONLY );
        read( pipe, msg, sizeof( copyjob_stats ) );
        close( pipe );

        return msg;
    }
}

copyjob_t* myDaemon( copyjob_stats* job ) {
    int pipe;

    if ( job != NULL ) {
        char* pipeName = "/tmp/daemonToClientFIFO";
        mkfifo( pipeName, 0666 );

        pipe = open( pipeName, O_WRONLY );
        write( pipe, job, sizeof( copyjob_stats ) );
        close( pipe );

        free( job );
        return NULL;
    }
    else {
        copyjob_t* msg = ( copyjob_t* ) calloc( 1, sizeof( copyjob_t ) );

        char* pipeName = "/tmp/clientToDaemonFIFO";
        mkfifo( pipeName, 0666 );

        pipe = open( pipeName, O_RDONLY );
        read( pipe, msg, sizeof( copyjob_t ) );
        close( pipe );

        return msg;
    }
}