#include "Base.h"
#include "Daemon.h"

//////////////////////////////////////////////////////////////////////////////
// BEG: Program Entrance
ORA_INT32 main()
{
    ORA_INT hLockFile = open( DAEMON_LOCK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR |S_IRGRP | S_IROTH );

    if( hLockFile < 0 )
    {
        printf( "Can't open file %s [%s]\n", DAEMON_LOCK_FILE, strerror( errno ) );
        return -1;
    }

    flock lock;
    lock.l_type   = F_WRLCK;
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    if( fcntl( hLockFile, F_SETLK, &lock ) < 0 )
    {
        if( errno == EACCES || errno == EAGAIN )
        {
            printf( "fastsetupd has been started, exit now.\n" );
            close( hLockFile );
            return -1;
        }

        printf( "Can't lock file %s [%s]\n", DAEMON_LOCK_FILE, strerror( errno ) );
        return -1;
    }

    ftruncate( hLockFile, 0 );
    ORA_CHAR buf[16];
    snprintf( buf, ORA_COUNT_OF( buf ), "%d", getpid() );
    buf[ ORA_COUNT_OF( buf ) - 1 ] = '\0';
    write( hLockFile, buf, strlen( buf ) + 1 );

    if( !ORAInitialize() )
    {
        printf( "Utils initialize failed! Application is exitting now." );
        close( hLockFile );
        return -1;
    }
    oralog_initialize( DAEMON_NAME, ORA_NULL );

    printf("Start Upgrade Daemon Successful, Wait Client Connect!!! \n");

    CDaemon *pDaemon = new CDaemon();
    if( pDaemon )
    {
        if( pDaemon->Start() )
            pDaemon->WaitForExit();

        delete pDaemon;
        pDaemon = ORA_NULL;
    }

    oralog_uninitialize();
    ORAUninitialize();
    close( hLockFile );
    return 0;
}
// END: Program Entrance
//////////////////////////////////////////////////////////////////////////////
