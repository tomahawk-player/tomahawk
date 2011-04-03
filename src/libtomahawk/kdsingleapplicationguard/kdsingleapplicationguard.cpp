#include "kdsingleapplicationguard.h"

#ifndef KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES
#define KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES 128
#endif

#ifndef KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE
#define KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE 1024
#endif

        

KDSingleApplicationGuard::Instance::Instance( const QStringList& args, qint64 p )
    : arguments( args ),
      pid( p )
{
}

#if QT_VERSION < 0x040400

class KDSingleApplicationGuard::Private
{
};

KDSingleApplicationGuard::KDSingleApplicationGuard( QCoreApplication*, Policy )
{
    qWarning( "KD Tools was compiled with a Qt version prior to 4.4. SingleApplicationGuard won't work." );
}

KDSingleApplicationGuard::~KDSingleApplicationGuard()
{
}

void KDSingleApplicationGuard::shutdownOtherInstances()
{
}

void KDSingleApplicationGuard::killOtherInstances()
{
}

void KDSingleApplicationGuard::timerEvent( QTimerEvent* )
{
}
#else

#include <QCoreApplication>
#include <QSharedMemory>

#include "kdsharedmemorylocker.h"
#include "kdlockedsharedmemorypointer.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifndef Q_WS_WIN
#include <csignal>
#endif

using namespace kdtools;

/*!
  \class KDSingleApplicationGuard KDSingleApplicationGuard
  \brief A guard to protect an application from having several instances.
 
  KDSingleApplicationGuard can be used to make sure only one instance of an
  application is running at the same time.
 
  \note As KDSingleApplicationGuard uses QSharedMemory Qt 4.4 or later is required
 */

/*!
  \fn void KDSingleApplicationGuard::instanceStarted()
  This signal is emitted by the primary instance when ever one other
  instance was started.
 */

/*!
  \fn void KDSingleApplicationGuard::instanceExited()
  This signal is emitted by the primary instance when ever one other
  instance was exited.
 */
    
/*!
  \fn void KDSingleApplicationGuard::becamePrimaryInstance()
  This signal is emitted, when the current running application gets the new
  primary application. The old primary application has quit.
 */

enum Command
{
    NoCommand = 0x00,
    ExitedInstance = 0x01,
    NewInstance = 0x02,
    FreeInstance = 0x04,
    ShutDownCommand = 0x08,
    KillCommand = 0x10,
    BecomePrimaryCommand = 0x20
};

Q_DECLARE_FLAGS( Commands, Command )
Q_DECLARE_OPERATORS_FOR_FLAGS( Commands )

struct ProcessInfo
{
    explicit ProcessInfo( Command c = FreeInstance, const QStringList& arguments = QStringList(), qint64 p = -1 )
        : command( c ),
          pid( p )
    {
        std::fill_n( commandline, KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE, '\0' );

        int argpos = 0;
        for( QStringList::const_iterator it = arguments.begin(); it != arguments.end(); ++it )
        {
            const QByteArray arg = it->toLatin1();
            const int count = qMin( KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE - argpos, arg.count() );
            std::copy( arg.begin(), arg.begin() + count, commandline + argpos );
            argpos += arg.count() + 1;  // makes sure there's a \0 between every parameter
        }
    }

    QStringList arguments() const
    {
        QStringList result;

        QByteArray arg;
        for( int i = 0; i < KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE; ++i )
        {
            if( commandline[ i ] == '\0' && !arg.isEmpty() )
            {
                result.push_back( QString::fromLatin1( arg ) );
                arg.clear();
            }
            else if( !commandline[ i ] == '\0' )
            {
                arg.push_back( commandline[ i ] );
            }
        }

        return result;
    }

    Commands command;
    char commandline[ KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE ];
    qint64 pid;
};

bool operator==( const ProcessInfo& lhs, const ProcessInfo& rhs )
{
    return lhs.command == rhs.command &&
           ::memcmp( lhs.commandline, rhs.commandline, KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE ) == 0;
}

bool operator!=( const ProcessInfo& lhs, const ProcessInfo& rhs )
{
    return !operator==( lhs, rhs );
}

/*!
  This struct contains information about the managed process system.
  \internal
 */
struct InstanceRegister
{
    InstanceRegister( KDSingleApplicationGuard::Policy policy = KDSingleApplicationGuard::NoPolicy )
        : policy( policy )
    {
        std::fill_n( info, KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES, ProcessInfo() );
        ::memcpy( magicCookie, "kdsingleapp", 12 );
    }

    /*!
      Returns wheter this register was properly initialized by the first instance.
      */
    bool isValid() const
    {
        return ::strcmp( magicCookie, "kdsingleapp" ) == 0;
    }

    ProcessInfo info[ KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES ];
    KDSingleApplicationGuard::Policy policy;
    char magicCookie[ 12 ];
};

bool operator==( const InstanceRegister& lhs, const InstanceRegister& rhs )
{
    if( lhs.policy != rhs.policy )
        return false;

    for( int i = 0; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
        if( lhs.info[ i ] != rhs.info[ i ] )
            return false;

    return true;
}

/*!
 \internal
 */
class KDSingleApplicationGuard::Private
{
public:
    Private( KDSingleApplicationGuard* qq )
        : q( qq ),
          id( -1 )
    {
        if( primaryInstance == 0 )
            primaryInstance = q;
    }

    ~Private()
    {
        if( primaryInstance == q )
            primaryInstance = 0;
    }

    void shutdownInstance()
    {
        KDLockedSharedMemoryPointer< InstanceRegister > instances( &q->d->mem );
        instances->info[ q->d->id ].command = ExitedInstance;

        if( q->isPrimaryInstance() )
        {
            // ohh... we need a new primary instance...
            for( int i = 1; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
            {
                if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance | ShutDownCommand | KillCommand ) ) == 0 )
                {
                    instances->info[ i ].command |= BecomePrimaryCommand;
                    return;
                }
            }
            // none found? then my species is dead :-(
        }
    }

    static KDSingleApplicationGuard* primaryInstance;

private:
    KDSingleApplicationGuard* const q;

public:
    Policy policy;
    QSharedMemory mem;
    int id;
};

KDSingleApplicationGuard* KDSingleApplicationGuard::Private::primaryInstance = 0;

#ifndef Q_WS_WIN
void SIGINT_handler( int sig )
{
    if( sig == SIGINT && KDSingleApplicationGuard::Private::primaryInstance != 0 )
        KDSingleApplicationGuard::Private::primaryInstance->d->shutdownInstance();
    ::exit( 1 );
}
#endif

/*!
  Creates a new KDSingleApplicationGuard guarding \a parent from mulitply instances.
  If \a policy is AutoKillOtherInstances (the default), all instances, which try to start, 
  are killed automatically and instanceStarted() is emitted.
  If \a policy is NoPolicy, the other instance will run and instanceStarted() is emitted.
 */
KDSingleApplicationGuard::KDSingleApplicationGuard( QCoreApplication* parent, Policy policy )
    : QObject( parent ),
      d( new Private( this ) )
{
    const QString name = parent->applicationName();
    Q_ASSERT_X( !name.isEmpty(), "KDSingleApplicationGuard::KDSingleApplicationGuard", "applicationName must not be emty" );
    d->mem.setKey( name );
   
    // if another instance crashed, the shared memory segment is still there on Unix
    // the following lines trigger deletion in that case
#ifndef Q_WS_WIN
    d->mem.attach();
    d->mem.detach();
#endif
    
    d->policy = policy;
            
    const bool created = d->mem.create( sizeof( InstanceRegister ) );
    if( !created )
    {
        if( !d->mem.attach() )
        {
            qWarning( "KDSingleApplicationGuard: Could neither create nor attach to shared memory segment." );
            return;
        }
        
        // lets wait till the other instance initialized the register
        bool initialized = false;
        while( !initialized )
        {
            const KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
            initialized = instances->isValid();
        }
    }


    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );

    if( !created )
    {
        // we're _not_ the first instance
        // but the 
        bool killOurSelf = false;
        
        // find a new slot...
        d->id = std::find( instances->info, instances->info + KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES, ProcessInfo() ) - instances->info;
        ProcessInfo& info = instances->info[ d->id ];
        info = ProcessInfo( NewInstance, parent->arguments(), QCoreApplication::applicationPid() );
        killOurSelf = instances->policy == AutoKillOtherInstances;
        d->policy = instances->policy;

        // but the signal that we tried to start was sent to the primary application
        if( killOurSelf )
        {
            info.command |= ExitedInstance;
            exit( 1 );
        }
    }
    else
    {
        // ok.... we are the first instance
        InstanceRegister reg( policy );        // create a new list
        d->id = 0;                             // our id = 0
        // and we've no command
        reg.info[ 0 ] = ProcessInfo( NoCommand, parent->arguments(), QCoreApplication::applicationPid() );
        *instances = reg;                      // push this is the process list into shared memory
    }

#ifndef Q_WS_WIN
    ::signal( SIGINT, SIGINT_handler );
#endif

    // now listen for commands
    startTimer( 250 );
}

/*!
  Destroys this SingleApplicationGuard.
  If this instance has been the primary instance and no other instance is existing anymore,
  the application is shut down completely. Otherwise the destructor selects another instance to 
  be the primary instances.
 */
KDSingleApplicationGuard::~KDSingleApplicationGuard()
{
    if( d->id == -1 )
        return;

    d->shutdownInstance();
}
 
/*!
  \property KDSingleApplicationGuard::primaryInstance
   Determines wheter this instance is the primary instance. 
   The primary instance is the first instance which was started or an instance which
   got selected by KDSingleApplicationGuard's destructor, when the primary instance was
   shut down.

   Get this property's value using %isPrimaryInstance(), and monitor changes to it
   using becamePrimaryInstance().
 */
bool KDSingleApplicationGuard::isPrimaryInstance() const
{
    return d->id == 0;
}

/*!
 \property KDSingleApplicationGuard::Policy
 Specifies the policy KDSingleApplicationGuard is using when new instances are started.
 This can only be set in the primary instance.

 Get this property's value using %policy(), set it using %setPolicy(), and monitor changes
 to it using policyChanged().
 */
KDSingleApplicationGuard::Policy KDSingleApplicationGuard::policy() const
{
    return d->policy;
}

void KDSingleApplicationGuard::setPolicy( Policy policy )
{
    Q_ASSERT( isPrimaryInstance() );
    if( d->policy == policy )
        return;

    d->policy = policy;
    emit policyChanged();
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    instances->policy = policy;
}

/*!
 Returns a list of all currently running instances.
 */
QList< KDSingleApplicationGuard::Instance > KDSingleApplicationGuard::instances() const
{
    QList< Instance > result;
    const KDLockedSharedMemoryPointer< InstanceRegister > instances( const_cast< QSharedMemory* >( &d->mem ) );
    for( int i = 0; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
    {
        const ProcessInfo& info = instances->info[ i ];
        if( ( info.command & ( FreeInstance | ExitedInstance ) ) == 0 )
            result.push_back( Instance( info.arguments(), info.pid ) );
    }
    return result;
}

/*! 
  Shuts down all other instances. This can only be called from the
  the primary instance.
  Shut down is done gracefully via QCoreApplication::quit().
 */
void KDSingleApplicationGuard::shutdownOtherInstances()
{
    Q_ASSERT( isPrimaryInstance() );
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    for( int i = 1; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
    {
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = ShutDownCommand;
    }
}

/*!
  Kills all other instances. This can only be called from the
  the primary instance.
  Killing is done via exit(1)
 */
void KDSingleApplicationGuard::killOtherInstances()
{
    Q_ASSERT( isPrimaryInstance() );
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    for( int i = 1; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
    {
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = KillCommand;
    }
}

/*!
  \reimp
 */
void KDSingleApplicationGuard::timerEvent( QTimerEvent* event )
{
    Q_UNUSED( event )

    if( isPrimaryInstance() )
    {
        // only the primary instance will get notified about new instances
        QList< Instance > exitedInstances;
        QList< Instance > startedInstances;

        {
            KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
            
            for( int i = 1; i < KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES; ++i )
            {
                ProcessInfo& info = instances->info[ i ];
                if( info.command & NewInstance ) 
                {
                    startedInstances.push_back( Instance( info.arguments(), info.pid ) );
                    info.command &= ~NewInstance;  // clear NewInstance flag
                }
                else if( info.command & ExitedInstance )
                {
                    exitedInstances.push_back( Instance( info.arguments(), info.pid ) );
                    info.command = FreeInstance;   // set FreeInstance flag
                }
            }
        }
     
        // one signal for every new instance - _after_ the memory segment was unlocked again
        for( QList< Instance >::const_iterator it = startedInstances.begin(); it != startedInstances.end(); ++it )
            emit instanceStarted( *it );
        for( QList< Instance >::const_iterator it = exitedInstances.begin(); it != exitedInstances.end(); ++it )
            emit instanceExited( *it );
    }
    else
    {
        // do we have a command?
        bool killOurSelf = false;
        bool shutDownOurSelf = false;
        bool policyDidChange = false;

        {
            KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );

            policyDidChange = instances->policy != d->policy;
            d->policy = instances->policy;

            if( instances->info[ d->id ].command & BecomePrimaryCommand )
            {
                // we became primary!
                instances->info[ 0 ] = instances->info[ d->id ]; 
                instances->info[ d->id ] = ProcessInfo();  // change our id to 0 and declare the old slot as free
                d->id = 0;
                emit becamePrimaryInstance();
            }

            killOurSelf = instances->info[ d->id ].command & KillCommand;            // check for kill command
            shutDownOurSelf = instances->info[ d->id ].command & ShutDownCommand;    // check for shut down command
            instances->info[ d->id ].command &= ~( KillCommand | ShutDownCommand | BecomePrimaryCommand );  // reset both flags
            if( killOurSelf )
            {
                instances->info[ d->id ].command |= ExitedInstance;  // upon kill, we have to set the ExitedInstance flag
                d->id = -1;                                          // becauso our d'tor won't be called anymore
            }
        }
   
        if( killOurSelf )  // kill our self takes precedence
            exit( 1 );
        else if( shutDownOurSelf )
            qApp->quit();
        else if( policyDidChange )
            emit policyChanged();
    }
}

#ifdef KDTOOLSCORE_UNITTESTS

#include <kdunittest/test.h>

#include <iostream>

#include <QtCore/QTime>
#include <QtCore/QUuid>
#include <QtTest/QSignalSpy>

Q_DECLARE_METATYPE( KDSingleApplicationGuard::Instance );

static void wait( int msec )
{
    QTime t;
    t.start();
    while( t.elapsed() < msec )
    {
        qApp->processEvents( QEventLoop::WaitForMoreEvents, msec - t.elapsed() );
    }
}

static std::ostream& operator<<( std::ostream& stream, const QStringList& list )
{
    stream << "QStringList(";
    for( QStringList::const_iterator it = list.begin(); it != list.end(); ++it )
    {
        stream << " " << it->toLocal8Bit().data();
        if( it + 1 != list.end() )
            stream << ",";
    }
    stream << " )";
    return stream;
}


KDAB_UNITTEST_SIMPLE( KDSingleApplicationGuard, "kdcoretools" ) {

    // set it to an unique name
    qApp->setApplicationName( QUuid::createUuid().toString() );

    qRegisterMetaType< KDSingleApplicationGuard::Instance >();

    KDSingleApplicationGuard* guard3 = 0;
    QSignalSpy* spy3 = 0;

    {
        KDSingleApplicationGuard guard1( qApp );
        assertEqual( guard1.policy(), KDSingleApplicationGuard::AutoKillOtherInstances );
        assertEqual( guard1.instances().count(), 1 );
        assertTrue( guard1.isPrimaryInstance() );

        guard1.setPolicy( KDSingleApplicationGuard::NoPolicy );
        assertEqual( guard1.policy(), KDSingleApplicationGuard::NoPolicy );

        QSignalSpy spy1( &guard1, SIGNAL( instanceStarted( KDSingleApplicationGuard::Instance ) ) );

        KDSingleApplicationGuard guard2( qApp );
        assertEqual( guard1.instances().count(), 2 );
        assertEqual( guard2.instances().count(), 2 );
        assertEqual( guard2.policy(), KDSingleApplicationGuard::NoPolicy );
        assertFalse( guard2.isPrimaryInstance() );

        wait( 1000 );

        assertEqual( spy1.count(), 1 );
        guard3 = new KDSingleApplicationGuard( qApp );
        spy3 = new QSignalSpy( guard3, SIGNAL( becamePrimaryInstance() ) );
        assertFalse( guard3->isPrimaryInstance() );
    }
        
    wait( 1000 );
    assertEqual( spy3->count(), 1 );
    assertEqual( guard3->instances().count(), 1 );
    assertTrue( guard3->isPrimaryInstance() );

    assertEqual( guard3->instances().first().arguments, qApp->arguments() );
    
    QSignalSpy spyStarted( guard3, SIGNAL( instanceStarted( KDSingleApplicationGuard::Instance ) ) );
    QSignalSpy spyExited( guard3, SIGNAL( instanceExited( KDSingleApplicationGuard::Instance ) ) );

    {
        KDSingleApplicationGuard guard1( qApp );
        KDSingleApplicationGuard guard2( qApp );

        wait( 1000 );

        assertEqual( spyStarted.count(), 2 );
    }
        
    wait( 1000 );
    assertEqual( spyExited.count(), 2 );

    delete spy3;
    delete guard3;
 }

#endif // KDTOOLSCORE_UNITTESTS

#endif // QT_VERSION < 0x040400
