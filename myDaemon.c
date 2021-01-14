#include "channel.h"
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

sem_t sem;
pthread_mutex_t mtx;
pthread_t threads[1024];
copyjob_stats* finishedThreads[1024];
pthread_t stackFinishedThreads[1024];
int stackPointer = 0;
int nrFinishedThreads = 0;
float progress[1024];
bool isPaused[1024];
int maxThreads = 4;
int maxJobs = 10;

float mini( float a, float b ) {
    return ( a < b ) ? a : b;
}

//(1) Creearea unui job de copiere:
void* copy_createjob( void* myJob ) {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    copyjob_t* job = ( copyjob_t* ) myJob;
    int fin, fout, nread;
    fin = open( job->src, O_RDONLY );
    fout = open( job->dst, O_WRONLY | O_CREAT | O_TRUNC, 0666 );
    char buff[4096];
    nread = read( fin, buff, 4096 );
    int totalBytesRead = 0;
    struct stat sb;
    stat( job->src, &sb );

    while ( nread > 0 ) {
        sleep( 5 );
        write( fout, buff, nread );
        totalBytesRead += nread;
        printf( "%d\n", job->threadId );
        progress[ job->threadId ] = mini( ( float ) totalBytesRead / sb.st_size, 1 );
        nread = read( fin, buff, 4096 );
        while ( isPaused[ job->threadId ] == true ) { ///asteptam daca threadul e pus pe pauza
            sleep( 1 );
        }
    }
    write( 1, "finished copy\n", 14 );
    if ( nread < 0 ) {
        perror( "read buf error\n" );
        pthread_cancel( pthread_self() );
    }
    copyjob_stats* sol = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
    sol->progres = 1;
    sol->threadId = job->threadId;
    strcpy( sol->state, "complet" );

    pthread_mutex_lock( &mtx );
    finishedThreads[ nrFinishedThreads++ ] = sol;
    stackFinishedThreads[ stackPointer++ ] = threads[ job->threadId ];
    threads[ job->threadId ] = 0;
    pthread_mutex_unlock( &mtx );

    free( job );
    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return NULL;
}

//(2) Anularea unui job de copiere:
void* copy_cancel( void* Job ) {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    copyjob_t* job = Job;
    pthread_mutex_lock( &mtx );
    if ( threads[ job->threadId ] ) {
        pthread_cancel( threads[ job->threadId ] );
        threads[ job->threadId ] = 0;
        progress[ job->threadId ] = 0;
    }
    stackFinishedThreads[ stackPointer++ ] = pthread_self();
    pthread_mutex_unlock( &mtx );
    free( job );
    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return NULL;
}

//(3) Intreruperea unui job de copiere:
void* copy_pause( void* Job ) {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    copyjob_t* job = ( copyjob_t* ) Job;
    pthread_mutex_lock( &mtx );
    if ( threads[ job->threadId ] > 0 )
        isPaused[ job->threadId ] = 1;
    stackFinishedThreads[ stackPointer++ ] = pthread_self();
    pthread_mutex_unlock( &mtx );
    free( job );
    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return NULL;
}

void* copy_resume( void* Job ) {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    copyjob_t* job = ( copyjob_t* ) Job;
    pthread_mutex_lock( &mtx );
    isPaused[ job->threadId ] = 0;
    stackFinishedThreads[ stackPointer++ ] = pthread_self();
    pthread_mutex_unlock( &mtx );
    free( job );
    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return NULL;
}

//(4) Obtinerea de informatii despre un job de copiere:
void* copy_stats( void* Job ) {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    copyjob_t* job = ( copyjob_t* ) Job;
    copyjob_stats* result = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
    pthread_mutex_lock( &mtx );
    if ( job->threadId >= 0 && job->threadId < maxJobs && threads[ job->threadId ] > 0 ) {
        result->progres = progress[ job->threadId ];
        strcpy( result->state, ( isPaused[ job->threadId ] == true ? "pauza" : ( progress[ job->threadId ] >= 1 ?
                                                                                 "complet" : "in progres" ) ) );
        result->threadId = job->threadId;
    }
    else {
        result->progres = 0;
        result->threadId = -1;
        strcpy( result->state, "Nu exista un asemenea job activ" );
    }
    stackFinishedThreads[ stackPointer++ ] = pthread_self();
    pthread_mutex_unlock( &mtx );
    free( job );
    copyjob_t* selfJob = calloc( 1, sizeof( copyjob_t ) );
    strcpy( selfJob->task, "self" );
    client( selfJob );

    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return result;
}


//(5) Enumerarea joburilor de copiere:
void* listjobs() {
    if ( sem_wait( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    pthread_mutex_lock( &mtx );
    copyjob_stats* jobFirst = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
    copyjob_stats* lastjob = NULL;
    copyjob_stats* firstJob = NULL;
    for ( int i = 0; i < maxJobs; i++ ) {
        if ( threads[ i ] != 0 ) {
            jobFirst->nrJobs++;
            copyjob_stats* jobCurent = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
            jobCurent->threadId = i;
            jobCurent->progres = progress[ i ];
            strcpy( jobCurent->state, ( isPaused[ jobCurent->threadId ] == true ? "pauza" :
                                        ( progress[ jobCurent->threadId ] >= 1 ? "complet" : "in progres" ) ) );
            jobCurent->nextJob = NULL;
            if ( firstJob == NULL ) {
                firstJob = jobCurent;
            }
            if ( lastjob != NULL ) {
                lastjob->nextJob = jobCurent;
            }
            lastjob = jobCurent;
        }
    }
    for ( int i = 0; i < nrFinishedThreads; i++ ) {
        jobFirst->nrJobs++;
        copyjob_stats* jobCurent = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
        jobCurent->threadId = -1;
        jobCurent->progres = finishedThreads[ i ]->progres;
        strcpy( jobCurent->state, finishedThreads[ i ]->state );
        jobCurent->nextJob = NULL;
        if ( firstJob == NULL ) {
            firstJob = jobCurent;
        }
        if ( lastjob != NULL ) {
            lastjob->nextJob = jobCurent;
        }
        lastjob = jobCurent;
        free( finishedThreads[ i ] );
    }
    nrFinishedThreads = 0;
    stackFinishedThreads[ stackPointer++ ] = pthread_self();
    pthread_mutex_unlock( &mtx );

    if ( jobFirst->nrJobs == 0 ) {
        strcpy( jobFirst->state, "Nu exista niciun job activ" );
    }
    jobFirst->nextJob = firstJob;

    copyjob_t* selfJob = calloc( 1, sizeof( copyjob_t ) );
    strcpy( selfJob->task, "self" );
    client( selfJob );

    if ( sem_post( &sem ) ) {
        perror( NULL );
        pthread_cancel( pthread_self() );
    }
    return jobFirst;
}


int main() {
    FILE* config = fopen( "config.in", "r" );
    if ( config != NULL ) {
        fscanf( config, "%d%d", &maxThreads, &maxJobs );
        fclose( config );
    }
    if ( pthread_mutex_init( &mtx, NULL ) ) {
        perror( NULL );
        return errno;
    }

    if ( sem_init( &sem, 0, maxThreads ) ) {
        perror( NULL );
        return errno;
    }
    while ( true ) {
        copyjob_stats* daemon_message;

        copyjob_t* client_message;
        client_message = myDaemon( NULL );
        if ( !strcmp( client_message->task, "copy" ) ) {
            int pozId = -1;
            while ( pozId < 0 ) {///trebuie sa gasim un thread liber
                for ( int i = 0; i < maxJobs && pozId < 0; i++ ) {
                    if ( threads[ i ] == 0 )
                        pozId = i;
                }
                if ( pozId < 0 )    //asteapta nitel daca nu ai gasit
                    sleep( 1 );
            }
            client_message->threadId = pozId; ///initializam cu threadul pe care l-am gasit
            if ( pthread_create( &threads[ pozId ], NULL, copy_createjob, client_message ) ) {
                perror( NULL );
                return errno;
            }
            write( 1, "Started copy\n", 13 );
            daemon_message = ( copyjob_stats* ) calloc( 1, sizeof( copyjob_stats ) );
            daemon_message->threadId = pozId;
            strcpy( daemon_message->state, "in progres" );
            daemon_message->progres = 0;
            myDaemon( daemon_message );
        }
        else if ( !strcmp( client_message->task, "pause" ) ) {
            if ( client_message->threadId >= 0 && client_message->threadId < maxJobs && threads[ client_message->threadId ] > 0 ) {
                pthread_t dummy;
                if ( pthread_create( &dummy, NULL, copy_pause, client_message ) ) {
                    perror( NULL );
                    return errno;
                }
            }
            else {
                free( client_message );
            }
        }
        else if ( !strcmp( client_message->task, "resume" ) ) {
            if ( client_message->threadId >= 0 && client_message->threadId < maxJobs && threads[ client_message->threadId ] > 0 ) {
                pthread_t dummy;
                if ( pthread_create( &dummy, NULL, copy_resume, client_message ) ) {
                    perror( NULL );
                    return errno;
                }
            }
            else {
                free( client_message );
            }
        }
        else if ( !strcmp( client_message->task, "cancel" ) ) {
            if ( client_message->threadId >= 0 && client_message->threadId < maxJobs && threads[ client_message->threadId ] > 0 ) {
                pthread_t dummy;
                if ( pthread_create( &dummy, NULL, copy_cancel, client_message ) ) {
                    perror( NULL );
                    return errno;
                }
            }
            else {
                free( client_message );
            }
        }
        else if ( !strcmp( client_message->task, "status" ) ) {
            pthread_t dummy;
            if ( pthread_create( &dummy, NULL, copy_stats, client_message ) ) {
                perror( NULL );
                return errno;
            }
        }
        else if ( !strcmp( client_message->task, "listJobs" ) ) {
            pthread_t dummy;
            if ( pthread_create( &dummy, NULL, listjobs, NULL ) ) {
                perror( NULL );
                return errno;
            }
            free( client_message );
        }

        else if ( !strcmp( client_message->task, "KILLDAEMON" ) ) {
            for ( int i = 0; i < maxJobs; i++ ) {
                if ( threads[ i ] != 0 ) {
                    pthread_cancel( threads[ i ] );
                    progress[ i ] = threads[ i ] = 0;
                }
            }
            for ( int i = 0; i < nrFinishedThreads; i++ )
                free( finishedThreads[ i ] );
            while ( stackPointer > 0 ) {
                void* result;
                if ( pthread_join( stackFinishedThreads[ --stackPointer ], &result ) ) {
                    perror( NULL );
                    return errno;
                }
                daemon_message = ( copyjob_stats* ) result;
                if ( daemon_message != NULL ) {
                    free( daemon_message );
                }
            }
            free( client_message );
            break;
        }
        else if ( !strcmp( client_message->task, "self" ) ) {
            while ( stackPointer > 0 ) {
                void* result;
                if ( pthread_join( stackFinishedThreads[ --stackPointer ], &result ) ) {///astept calcularea elem respectiv
                    perror( NULL );
                    return errno;
                }
                daemon_message = ( copyjob_stats* ) result;
                if ( daemon_message != NULL ) {
                    if ( daemon_message->nrJobs > 0 ) {///suntem obligati sa serializam lista la transmiterea prin pipe
                        copyjob_stats* nextPtr = daemon_message->nextJob;
                        daemon_message->nextJob = NULL;
                        myDaemon( daemon_message );
                        while ( nextPtr != NULL ) {
                            daemon_message = nextPtr;
                            nextPtr = nextPtr->nextJob;
                            daemon_message->nextJob = NULL;
                            myDaemon( daemon_message );
                        }
                    }
                    else {
                        myDaemon( daemon_message );
                    }
                }
            }
            free( client_message );
        }
    }

    sem_destroy( &sem );
    pthread_mutex_destroy( &mtx );
    return 0;
}