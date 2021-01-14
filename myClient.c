#include "channel.h"

int main( int argc, char* argv[] ) {
    if ( argc >= 2 ) {
        copyjob_t* messageClient = calloc( 1, sizeof( copyjob_t ) );
        strcpy( messageClient->task, argv[ 1 ] );

        if ( !strcmp( messageClient->task, "copy" ) && argc == 4 ) {
            strcpy( messageClient->src, argv[ 2 ] );
            strcpy( messageClient->dst, argv[ 3 ] );
            client( messageClient );
            copyjob_stats* response = client( NULL );
            printf( "threadId atribuit: %d\n", response->threadId );
            free( response );
        }
        else if ( !strcmp( messageClient->task, "cancel" ) && argc == 3 ) {
            messageClient->threadId = atoi( argv[ 2 ] );
            client( messageClient );
        }
        else if ( !strcmp( messageClient->task, "pause" ) && argc == 3 ) {
            messageClient->threadId = atoi( argv[ 2 ] );
            client( messageClient );
        }
        else if ( !strcmp( messageClient->task, "resume" ) && argc == 3 ) {
            messageClient->threadId = atoi( argv[ 2 ] );
            client( messageClient );
        }
        else if ( !strcmp( messageClient->task, "status" ) && argc == 3 ) {
            messageClient->threadId = atoi( argv[ 2 ] );
            client( messageClient );
            copyjob_stats* response = client( NULL );
            printf( "%d\n%f%%\n%s\n...\n\n", response->threadId, response->progres * 100, response->state );
            free( response );
        }
        else if ( !strcmp( messageClient->task, "listJobs" ) && argc == 2 ) {
            client( messageClient );
            copyjob_stats* response = client( NULL );
            int nr = response->nrJobs;
            free( response );
            for ( int i = 0; i < nr; i++ ) {
                response = client( NULL );
                printf( "%d\n%f%%\n%s\n...\n\n", response->threadId, response->progres * 100, response->state );
                free( response );
            }
        }
        else if ( !strcmp( messageClient->task, "KILLDAEMON" ) && argc == 2 ) {
            client( messageClient );
        }
    }

    return 0;
}