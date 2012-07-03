#include "kdsingleapplicationguard.h"

#if QT_VERSION >= 0x040400 || defined(DOXYGEN_RUN)
#ifndef QT_NO_SHAREDMEMORY

#include "kdsharedmemorylocker.h"
#include "kdlockedsharedmemorypointer.h"

#include <QVector>
#include <QCoreApplication>
#include <QSharedMemory>
#include <QSharedData>
#include <QBasicTimer>
#include <QTime>

#include <algorithm>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <cassert>

#ifndef Q_WS_WIN
#include <csignal>
#include <unistd.h>
#endif

#ifdef Q_WS_WIN
#include <windows.h>
#ifndef _SSIZE_T_DEFINED
typedef signed int ssize_t;
#endif
#endif

using namespace kdtools;

#ifndef KDSINGLEAPPLICATIONGUARD_TIMEOUT_SECONDS
#define KDSINGLEAPPLICATIONGUARD_TIMEOUT_SECONDS 10
#endif

#ifndef KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES
#define KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES 10
#endif

#ifndef KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE
#define KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE 32768
#endif

static unsigned int KDSINGLEAPPLICATIONGUARD_SHM_VERSION = 0;

Q_GLOBAL_STATIC_WITH_ARGS( int, registerInstanceType,
                           (qRegisterMetaType<KDSingleApplicationGuard::Instance>()) )

/*!
  \class KDSingleApplicationGuard::Instance
  \relates KDSingleApplicationGuard
  \ingroup core
  \brief Information about instances a KDSingleApplicationGuard knows about

  Instance represents instances of applications under
  KDSingleApplicationGuard protection, and allows access to their
  pid() and the arguments() they were started with.
*/

class KDSingleApplicationGuard::Instance::Private : public QSharedData {
    friend class ::KDSingleApplicationGuard::Instance;
public:
    Private( const QStringList & args, bool truncated, qint64 pid )
        : pid( pid ), arguments( args ), truncated( truncated ) {}

private:
    qint64 pid;
    QStringList arguments;
    bool truncated;
};

struct ProcessInfo;

/*!
 \internal
 */
class KDSingleApplicationGuard::Private
{
    friend class ::KDSingleApplicationGuard;
    friend class ::KDSingleApplicationGuard::Instance;
    friend struct ::ProcessInfo;
    KDSingleApplicationGuard * const q;
public:
    Private( Policy policy, KDSingleApplicationGuard* qq );
    ~Private();

    void create( const QStringList& arguments );

    bool checkOperational( const char * function, const char * act ) const;
    bool checkOperationalPrimary( const char * function, const char * act ) const;

    struct segmentheader
    {
        size_t size : 16;
    };

    static void sharedmem_free( char* );
    static char* sharedmem_malloc( size_t size );

private:
    void shutdownInstance();
    void poll();

private:
    static KDSingleApplicationGuard* primaryInstance;

private:
    QBasicTimer timer;
    QSharedMemory mem;
    int id;
    Policy policy;
    bool operational;
    bool exitRequested;
};

/*!
  \internal
*/
KDSingleApplicationGuard::Instance::Instance( const QStringList & args, bool truncated, qint64 p )
    : d( new Private( args, truncated, p ) )
{
    d->ref.ref();
    (void)registerInstanceType();
}

/*!
  Default constructor. Constructs in Instance that is \link isNull()
  null\endlink.

  \sa isNull()
*/
KDSingleApplicationGuard::Instance::Instance() : d( 0 ) {}

/*!
  Copy constructor.
*/
KDSingleApplicationGuard::Instance::Instance( const Instance & other )
    : d( other.d )
{
    if ( d )
        d->ref.ref();
}

/*!
  Destructor.
*/
KDSingleApplicationGuard::Instance::~Instance()
{
    if ( d && !d->ref.deref() )
        delete d;
}

/*!
  \fn KDSingleApplicationGuard::Instance::swap( Instance & other )

  Swaps the contents of this and \a other.

  This function never throws exceptions.
*/

/*!
  \fn KDSingleApplicationGuard::Instance::operator=( Instance other )

  Assigns the contents of \a other to this.

  This function is strongly exception-safe.
*/

/*!
  \fn std::swap( KDSingleApplicationGuard::Instance & lhs, KDSingleApplicationGuard::Instance & rhs )
  \relates KDSingleApplicationGuard::Instance

  Specialisation of std::swap() for
  KDSingleApplicationGuard::Instance. Calls swap().
*/

/*!
  \fn qSwap( KDSingleApplicationGuard::Instance & lhs, KDSingleApplicationGuard::Instance & rhs )
  \relates KDSingleApplicationGuard::Instance

  Specialisation of qSwap() for
  KDSingleApplicationGuard::Instance. Calls swap().
*/

/*!
  \fn KDSingleApplicationGuard::Instance::isNull() const

  Returns whether this instance is null.
*/

/*!
  Returns whether this instance is valid. A valid instance is neither
  null, nor does it have a negative PID.
*/
bool KDSingleApplicationGuard::Instance::isValid() const
{
    return d && d->pid >= 0 ;
}

/*!
  Returns whether the #arguments are complete (\c false) or not (\c
  true), e.g. because they have been truncated due to limited storage
  space.

  \sa arguments()
*/
bool KDSingleApplicationGuard::Instance::areArgumentsTruncated() const
{
    return d && d->truncated;
}

/*!
  Returns the arguments that this instance was started with.

  \sa areArgumentsTruncated()
*/
const QStringList & KDSingleApplicationGuard::Instance::arguments() const
{
    if ( d )
        return d->arguments;
    static const QStringList empty;
    return empty;
}

/*!
  Returns the process-id (PID) of this instance.
*/
qint64 KDSingleApplicationGuard::Instance::pid() const
{
    if ( d )
        return d->pid;
    else
        return -1;
}

/*!
  \class KDSingleApplicationGuard KDSingleApplicationGuard
  \ingroup core
  \brief A guard to protect an application from having several instances.

  KDSingleApplicationGuard can be used to make sure only one instance of an
  application is running at the same time.

  \note As KDSingleApplicationGuard currently uses QSharedMemory, Qt
  4.4 or later is required.
 */

/*!
  \fn void KDSingleApplicationGuard::instanceStarted(const KDSingleApplicationGuard::Instance & instance)

  This signal is emitted by the primary instance whenever another
  instance \a instance started.
 */

/*!
  \fn void KDSingleApplicationGuard::instanceExited(const KDSingleApplicationGuard::Instance & instance)

  This signal is emitted by the primary instance whenever another
  instance \a instance exited.
 */

/*!
  \fn void KDSingleApplicationGuard::raiseRequested()

  This signal is emitted when the current running application is requested
  to raise its main window.
*/

/*!
  \fn void KDSingleApplicationGuard::exitRequested()

  This signal is emitted when the current running application has been asked to exit
  by calling kill on the instance.
*/

/*!
  \fn void KDSingleApplicationGuard::becamePrimaryInstance()

  This signal is emitted when the current running application becomes
  the new primary application. The old primary application has quit.
 */

/*!
  \fn void KDSingleApplicationGuard::becameSecondaryInstance()

  This signal is emmited when the primary instance became secondary instance.
  This happens when the instance doesn't update its status for some (default 10) seconds. Another instance
  got primary instance in that case.
  */

/*!
  \fn void KDSingleApplicationGuard::policyChanged( KDSingleApplicationGuard::Policy policy )

  This signal is emitted when the #policy of the system changes.
*/

enum Command
{
    NoCommand = 0x00,
    ExitedInstance = 0x01,
    NewInstance = 0x02,
    FreeInstance = 0x04,
    ShutDownCommand = 0x08,
    KillCommand = 0x10,
    BecomePrimaryCommand = 0x20,
    RaiseCommand = 0x40
};

static const quint16 PrematureEndOfOptions = -1;
static const quint16 RegularEndOfOptions   = -2;

struct ProcessInfo
{
    static const size_t MarkerSize = sizeof(quint16);

    explicit ProcessInfo( Command c = FreeInstance, const QStringList& arguments = QStringList(), qint64 p = -1 )
        : pid( p ),
          command( c ),
          timestamp( 0 ),
          commandline( 0 )
    {
        setArguments( arguments );
    }

    void setArguments( const QStringList & arguments );
    QStringList arguments( bool * prematureEnd  ) const;

    qint64 pid;
    quint32 command;
    quint32 timestamp;
    char* commandline;
};

static inline bool operator==( const ProcessInfo & lhs, const ProcessInfo & rhs )
{
    return lhs.command == rhs.command &&
           ( lhs.commandline == rhs.commandline || ( lhs.commandline != 0 && rhs.commandline != 0 && ::strcmp( lhs.commandline, rhs.commandline ) == 0 ) );
}

static inline bool operator!=( const ProcessInfo & lhs, const ProcessInfo & rhs )
{
    return !operator==( lhs, rhs );
}

/*!
  This struct contains information about the managed process system.
  \internal
 */
struct InstanceRegister
{
    explicit InstanceRegister( KDSingleApplicationGuard::Policy policy = KDSingleApplicationGuard::NoPolicy )
        : policy( policy ),
          maxInstances( KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES ),
          version( 0 )
    {
        std::fill_n( commandLines, KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE, 0 );
        ::memcpy( magicCookie, "kdsingleapp", 12 );
    }

    /*!
      Returns whether this register was properly initialized by the first instance.
      */
    bool isValid() const
    {
        return ::strcmp( magicCookie, "kdsingleapp" ) == 0;
    }

    char magicCookie[ 12 ];
    unsigned int policy  :  8;
    quint32 maxInstances : 20;
    unsigned int version :  4;
    ProcessInfo info[ KDSINGLEAPPLICATIONGUARD_NUMBER_OF_PROCESSES ];

    char commandLines[ KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE ];

    Q_DISABLE_COPY( InstanceRegister )
};

void ProcessInfo::setArguments( const QStringList & arguments )
{
    if( commandline != 0 )
        KDSingleApplicationGuard::Private::sharedmem_free( commandline );

    commandline = 0;
    if( arguments.isEmpty() )
        return;

    size_t totalsize = MarkerSize;
    Q_FOREACH( const QString& arg, arguments )
    {
        const QByteArray utf8 = arg.toUtf8();
        totalsize += utf8.size() + MarkerSize;
    }
    InstanceRegister* const reg = reinterpret_cast<InstanceRegister*>( KDSingleApplicationGuard::Private::primaryInstance->d->mem.data() );
    this->commandline = KDSingleApplicationGuard::Private::sharedmem_malloc( totalsize );
    if( this->commandline == 0 )
    {
        qWarning("KDSingleApplicationguard: out of memory when trying to save arguments.\n");
        return;
    }

    char* const commandline = this->commandline + reinterpret_cast<qptrdiff>(reg->commandLines);

    int argpos = 0;
    Q_FOREACH( const QString & arg, arguments )
    {
        const QByteArray utf8 = arg.toUtf8();
        const int required = MarkerSize + utf8.size() + MarkerSize ;
        const int available = KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE - argpos ;
        if ( required > available || utf8.size() > std::numeric_limits<quint16>::max() ) {
            // write a premature-eoo marker, and quit
            memcpy( commandline + argpos, &PrematureEndOfOptions, MarkerSize );
            argpos += MarkerSize;
            qWarning( "KDSingleApplicationGuard: argument list is too long (bytes required: %d, used: %d, available: %d",
                      required, argpos - 2, available );
            return;
        } else {
            const quint16 len16 = utf8.size();
            // write the size of the data...
            memcpy( commandline + argpos, &len16, MarkerSize );
            argpos += MarkerSize;
            // then the data
            memcpy( commandline + argpos, utf8.data(), len16 );
            argpos += len16;
        }
    }
    const ssize_t available = KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE - argpos;
    assert( available >= static_cast<ssize_t>( MarkerSize ) );
    memcpy( commandline + argpos, &RegularEndOfOptions, MarkerSize );
    argpos += MarkerSize;
}

QStringList ProcessInfo::arguments( bool * prematureEnd  ) const
{
    QStringList result;
    if( commandline == 0 )
    {
        if( prematureEnd )
            *prematureEnd = true;
        return result;
    }

    InstanceRegister* const reg = reinterpret_cast<InstanceRegister*>( KDSingleApplicationGuard::Private::primaryInstance->d->mem.data() );
    const char* const commandline = this->commandline + reinterpret_cast<qptrdiff>(reg->commandLines);

    int argpos = 0;
    while ( true ) {
        const int available = KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE - argpos ;
        assert( available >= 2 );

        quint16 marker;
        memcpy( &marker, commandline + argpos, MarkerSize );
        argpos += MarkerSize;

        if ( marker == PrematureEndOfOptions ) {
            if ( prematureEnd ) *prematureEnd = true;
            break;
        }
        if ( marker == RegularEndOfOptions ) {
            if ( prematureEnd ) *prematureEnd = false;
            break;
        }

        const int requested = MarkerSize + marker + MarkerSize ;
        if ( requested > available ) {
            const long long int p = pid;
            qWarning( "KDSingleApplicationGuard: inconsistency detected when parsing command-line argument for process %lld", p );
            if ( prematureEnd ) *prematureEnd = true;
            break;
        }

        result.push_back( QString::fromUtf8( commandline + argpos, marker ) );
        argpos += marker;
    }

    return result;
}

KDSingleApplicationGuard::Private::~Private()
{
    if( primaryInstance == q )
        primaryInstance = 0;
}

bool KDSingleApplicationGuard::Private::checkOperational( const char * function, const char * act ) const
{
    assert( function );
    assert( act );
    if ( !operational )
        qWarning( "KDSingleApplicationGuard::%s: need to be operational to %s", function, act );
    return operational;
}

bool KDSingleApplicationGuard::Private::checkOperationalPrimary( const char * function, const char * act ) const
{
    if ( !checkOperational( function, act ) )
        return false;
    if ( id != 0 )
        qWarning( "KDSingleApplicationGuard::%s: need to be primary to %s", function, act );
    return id == 0;
}

struct segmentheader
{
    size_t size : 16;
};

void KDSingleApplicationGuard::Private::sharedmem_free( char* pointer )
{
    InstanceRegister* const reg = reinterpret_cast<InstanceRegister*>( KDSingleApplicationGuard::Private::primaryInstance->d->mem.data() );
    char* const heap = reg->commandLines;
    char* const heap_ptr = heap + reinterpret_cast<qptrdiff>(pointer) - sizeof( segmentheader );
    const segmentheader* const header = reinterpret_cast< const segmentheader* >( heap_ptr );
    const size_t size = header->size;

    char* end = heap + KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE;

    std::copy( heap_ptr + size, end, heap_ptr );
    std::fill( end - size, end, 0 );

    for( uint i = 0; i < reg->maxInstances; ++i )
    {
        if( reg->info[ i ].commandline > pointer )
            reg->info[ i ].commandline -= size + sizeof( segmentheader );
    }
}

char* KDSingleApplicationGuard::Private::sharedmem_malloc( size_t size )
{
    InstanceRegister* const reg = reinterpret_cast<InstanceRegister*>( KDSingleApplicationGuard::Private::primaryInstance->d->mem.data() );
    char* heap = reg->commandLines;

    while( heap + sizeof( segmentheader ) + size < reg->commandLines + KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE )
    {
        segmentheader* const header = reinterpret_cast< segmentheader* >( heap );
        if( header->size == 0 )
        {
            header->size = size;
            return heap + sizeof( segmentheader ) - reinterpret_cast<qptrdiff>(reg->commandLines);
        }
        heap += sizeof( header ) + header->size;
    }
    return 0;
}

void KDSingleApplicationGuard::Private::shutdownInstance()
{
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &q->d->mem );
    instances->info[ q->d->id ].command |= ExitedInstance;

    if( q->isPrimaryInstance() )
    {
        // ohh... we need a new primary instance...
        for ( int i = 1, end = instances->maxInstances ; i  < end ; ++i )
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

KDSingleApplicationGuard* KDSingleApplicationGuard::Private::primaryInstance = 0;

/*!
  Requests that the instance kills itself (by emitting exitRequested).

  If the instance has since exited, does nothing.

  \sa shutdown(), raise()
*/
void KDSingleApplicationGuard::Instance::kill()
{
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &KDSingleApplicationGuard::Private::primaryInstance->d->mem );
    for ( int i = 0, end = instances->maxInstances ; i < end ; ++i )
    {
        if( instances->info[ i ].pid != d->pid )
           continue;
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = KillCommand;
    }
}

/*!
  Requests that the instance shuts itself down (by calling QCoreApplication::quit()).

  If the instance has since exited, does nothing.

  \sa kill(), raise()
*/
void KDSingleApplicationGuard::Instance::shutdown()
{
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &KDSingleApplicationGuard::Private::primaryInstance->d->mem );
    for ( int i = 0, end = instances->maxInstances ; i < end ; ++i )
    {
        if( instances->info[ i ].pid != d->pid )
           continue;
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = ShutDownCommand;
    }
}

/*!

  Requests that the instance raises its main window.

  The effects are implementation-defined: the KDSingleApplicationGuard
  corresponding to the instance will emit its \link
  KDSingleApplicationGuard::raiseRequested() raiseRequested()\endlink
  signal.

  If the instance has since exited, does nothing.

  \sa kill(), shutdown()
*/
void KDSingleApplicationGuard::Instance::raise()
{
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &KDSingleApplicationGuard::Private::primaryInstance->d->mem );
    for ( int i = 0, end = instances->maxInstances ; i < end ; ++i )
    {
        if( instances->info[ i ].pid != d->pid )
           continue;
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = RaiseCommand;
    }
}


#ifndef Q_WS_WIN
// static
void KDSingleApplicationGuard::SIGINT_handler( int sig )
{
    if( sig == SIGINT && Private::primaryInstance != 0 )
        Private::primaryInstance->d->shutdownInstance();
    ::exit( 1 );
}
#endif

/*!
  \enum KDSingleApplicationGuard::Policy

  Defines the policy that a KDSingleApplicationGuard can enforce:
*/

/*!
  \var KDSingleApplicationGuard::NoPolicy

  instanceStarted() is emitted, and the new instance allowed to continue.
*/

/*!
  \var KDSingleApplicationGuard::AutoKillOtherInstances

  instanceStarted() is emitted, and the new instance is killed (Instance::kill()).
*/

/*!
  Creates a new KDSingleApplicationGuard with arguments
  QCoreApplication::arguments() and policy AutoKillOtherInstances,
  passing \a parent to the base class constructor, as usual.
*/
KDSingleApplicationGuard::KDSingleApplicationGuard( QObject * parent )
    : QObject( parent ), d( new Private( AutoKillOtherInstances, this ) )
{
    d->create( QCoreApplication::arguments() );
}

/*!
  Creates a new KDSingleApplicationGuard with arguments
  QCoreApplication::arguments() and policy \a policy, passing \a
  parent to the base class constructor, as usual.
*/
KDSingleApplicationGuard::KDSingleApplicationGuard( Policy policy, QObject * parent )
    : QObject( parent ), d( new Private( policy, this ) )
{
    d->create( QCoreApplication::arguments() );
}

/*!
  Creates a new KDSingleApplicationGuard with arguments \a arguments
  and policy AutoKillOtherInstances, passing \a parent to the base
  class constructor, as usual.
*/
KDSingleApplicationGuard::KDSingleApplicationGuard( const QStringList & arguments, QObject * parent )
    : QObject( parent ), d( new Private( AutoKillOtherInstances, this ) )
{
    d->create( arguments );
}

/*!
  Creates a new KDSingleApplicationGuard with arguments \a arguments
  and policy \a policy, passing \a parent to the base class
  constructor, as usual.
*/
KDSingleApplicationGuard::KDSingleApplicationGuard( const QStringList & arguments, Policy policy, QObject * parent )
    : QObject( parent ), d( new Private( policy, this ) )
{
    d->create( arguments );
}

KDSingleApplicationGuard::Private::Private( Policy policy_, KDSingleApplicationGuard * qq )
    : q( qq ),
      id( -1 ),
      policy( policy_ ),
      operational( false ),
      exitRequested( false )
{
}

void KDSingleApplicationGuard::Private::create( const QStringList & arguments )
{
    if ( !QCoreApplication::instance() ) {
        qWarning( "KDSingleApplicationGuard: you need to construct a Q(Core)Application before you can construct a KDSingleApplicationGuard" );
        return;
    }

    const QString name = QCoreApplication::applicationName();
    if ( name.isEmpty() ) {
        qWarning( "KDSingleApplicationGuard: QCoreApplication::applicationName must not be emty" );
        return;
    }

    (void)registerInstanceType();
    if ( primaryInstance == 0 )
        primaryInstance = q;

    mem.setKey( name );

    // if another instance crashed, the shared memory segment is still there on Unix
    // the following lines trigger deletion in that case
#ifndef Q_WS_WIN
    mem.attach();
    mem.detach();
#endif

    const bool created = mem.create( sizeof( InstanceRegister ) );
    if( !created )
    {
        QString errorMsg;
        if( mem.error() != QSharedMemory::NoError && mem.error() != QSharedMemory::AlreadyExists )
            errorMsg += QString::fromLatin1( "QSharedMemomry::create() failed: %1" ).arg( mem.errorString() );

        if( !mem.attach() )
        {
            if( mem.error() != QSharedMemory::NoError )
                errorMsg += QString::fromLatin1( "QSharedMemomry::attach() failed: %1" ).arg( mem.errorString() );

            qWarning( "KDSingleApplicationGuard: Could neither create nor attach to shared memory segment." );
            qWarning( "%s\n",  errorMsg.toLocal8Bit().constData() );
            return;
        }

        const int maxWaitMSecs = 1000 * 60; // stop waiting after 60 seconds
        QTime waitTimer;
        waitTimer.start();

        // lets wait till the other instance initialized the register
        bool initialized = false;
        while( !initialized && waitTimer.elapsed() < maxWaitMSecs )
        {
            const KDLockedSharedMemoryPointer< InstanceRegister > instances( &mem );
            initialized = instances->isValid();
#ifdef Q_WS_WIN
            ::Sleep(20);
#else
            usleep(20000);
#endif
        }

        const KDLockedSharedMemoryPointer< InstanceRegister > instances( &mem );
        if ( instances->version != 0 ) {
            qWarning( "KDSingleApplicationGuard: Detected version mismatch. "
                      "Highest supported version: %ud, actual version: %ud",
                      KDSINGLEAPPLICATIONGUARD_SHM_VERSION, instances->version );
            return;
        }

    }


    KDLockedSharedMemoryPointer< InstanceRegister > instances( &mem );

    if( !created )
    {
        assert( instances->isValid() );

        // we're _not_ the first instance
        // but the
        bool killOurSelf = false;

        // find a new slot...
        id = std::find( instances->info, instances->info + instances->maxInstances, ProcessInfo() ) - instances->info;
        ProcessInfo& info = instances->info[ id ];
        info = ProcessInfo( NewInstance, arguments, QCoreApplication::applicationPid() );
        killOurSelf = instances->policy == AutoKillOtherInstances;
        policy = static_cast<Policy>( instances->policy );

        // but the signal that we tried to start was sent to the primary application
        if( killOurSelf )
            exitRequested = true;
    }
    else
    {
        // ok.... we are the first instance
        new ( instances.get() ) InstanceRegister( policy ); // create a new list (in shared memory)
        id = 0;                                             // our id = 0
        // and we've no command
        instances->info[ 0 ] = ProcessInfo( NoCommand, arguments, QCoreApplication::applicationPid() );
    }

#ifndef Q_WS_WIN
    ::signal( SIGINT, SIGINT_handler );
#endif

    // now listen for commands
    timer.start( 750, q );

    operational = true;
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
  \property KDSingleApplicationGuard::operational

  Contains whether this KDSingleApplicationGuard is operational.

  A non-operational KDSingleApplicationGuard cannot be used in any meaningful way.

  Reasons for a KDSingleApplicationGuard being non-operational include:
  \li it was constructed before QApplication (or at least QCoreApplication) was constructed
  \li it failed to create or attach to the shared memory segment that is used for communication

  Get this property's value using %isOperational().
*/
bool KDSingleApplicationGuard::isOperational() const
{
    return d->operational;
}

/*!
  \property KDSingleApplicationGuard::exitRequested

  Contains wheter this istance has been requested to exit. This will happen when this instance
  was just started, but the policy is AutoKillOtherInstances or by explicitely calling kill on
  this instance().

  Get this property's value using %isExitRequested().
*/
bool KDSingleApplicationGuard::isExitRequested() const
{
    return d->exitRequested;
};

/*!
  \property KDSingleApplicationGuard::primaryInstance

   Contains whether this instance is the primary instance.

   The primary instance is the first instance which was started or else the instance which
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
 \property KDSingleApplicationGuard::policy
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
    if ( !d->checkOperationalPrimary( "setPolicy", "change the policy" ) )
        return;

    if( d->policy == policy )
        return;

    d->policy = policy;
    emit policyChanged( policy );
    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    instances->policy = policy;
}

/*!
 Returns a list of all currently running instances.
 */
QVector<KDSingleApplicationGuard::Instance>
KDSingleApplicationGuard::instances() const
{
    if ( !d->checkOperational( "instances", "report on other instances" ) )
        return QVector<Instance>();

    if ( Private::primaryInstance == 0 ) {
        Private::primaryInstance = const_cast<KDSingleApplicationGuard*>( this );
    }

    QVector<Instance> result;
    const KDLockedSharedMemoryPointer< InstanceRegister > instances( const_cast< QSharedMemory* >( &d->mem ) );
    for ( int i = 0, end = instances->maxInstances ; i < end ; ++i )
    {
        const ProcessInfo& info = instances->info[ i ];
        if( ( info.command & ( FreeInstance | ExitedInstance ) ) == 0 )
        {
            bool truncated;
            const QStringList arguments = info.arguments( &truncated );
            result.push_back( Instance( arguments, truncated, info.pid ) );
        }
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
    if ( !d->checkOperationalPrimary( "shutdownOtherInstances", "shut other instances down" ) )
        return;

    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    for ( int i = 1, end = instances->maxInstances ; i < end ; ++i )
    {
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = ShutDownCommand;
    }
}

/*!
  Kills all other instances. This can only be called from the
  the primary instance.
  Killing is done via emitting exitRequested. It's up to the receiving
  instance to react properly.
 */
void KDSingleApplicationGuard::killOtherInstances()
{
    if ( !d->checkOperationalPrimary( "killOtherInstances", "kill other instances" ) )
        return;

    KDLockedSharedMemoryPointer< InstanceRegister > instances( &d->mem );
    for ( int i = 1, end = instances->maxInstances ; i < end ; ++i )
    {
        if( ( instances->info[ i ].command & ( FreeInstance | ExitedInstance ) ) == 0 )
            instances->info[ i ].command = KillCommand;
    }
}

bool KDSingleApplicationGuard::event( QEvent * event )
{
    if ( event->type() == QEvent::Timer ) {
        const QTimerEvent * const te = static_cast<QTimerEvent*>( event );
        if ( te->timerId() == d->timer.timerId() ) {
            d->poll();
            return true;
        }
    }
    return QObject::event( event );
}

void KDSingleApplicationGuard::Private::poll() {

    const quint32 now = QDateTime::currentDateTime().toTime_t();

    if ( primaryInstance == 0 ) {
        primaryInstance = q;
    }

    if ( q->isPrimaryInstance() )
    {
        // only the primary instance will get notified about new instances
        QVector< Instance > exitedInstances;
        QVector< Instance > startedInstances;

        {
            KDLockedSharedMemoryPointer< InstanceRegister > instances( &mem );

            if( instances->info[ id ].pid != QCoreApplication::applicationPid() )
            {
                for ( int i = 1, end = instances->maxInstances ; i < end && id == 0 ; ++i )
                {
                    if( instances->info[ i ].pid == QCoreApplication::applicationPid() )
                        id = i;
                }
                emit q->becameSecondaryInstance();
                return;
            }

            instances->info[ id ].timestamp = now;

            for ( int i = 1, end = instances->maxInstances ; i < end ; ++i )
            {
                ProcessInfo& info = instances->info[ i ];
                if( info.command & NewInstance )
                {
                    bool truncated;
                    const QStringList arguments = info.arguments( &truncated );
                    startedInstances.push_back( Instance( arguments, truncated, info.pid ) );
                    info.command &= ~NewInstance;  // clear NewInstance flag
                }
                if( info.command & ExitedInstance )
                {
                    bool truncated;
                    const QStringList arguments = info.arguments( &truncated );
                    exitedInstances.push_back( Instance( arguments, truncated, info.pid ) );
                    info.command = FreeInstance;   // set FreeInstance flag
                }
            }
        }

        // one signal for every new instance - _after_ the memory segment was unlocked again
        for( QVector< Instance >::const_iterator it = startedInstances.constBegin(); it != startedInstances.constEnd(); ++it )
            emit q->instanceStarted( *it );
        for( QVector< Instance >::const_iterator it = exitedInstances.constBegin(); it != exitedInstances.constEnd(); ++it )
            emit q->instanceExited( *it );
    }
    else
    {
        // do we have a command?
        bool killOurSelf = false;
        bool shutDownOurSelf = false;
        bool policyDidChange = false;

        {
            KDLockedSharedMemoryPointer< InstanceRegister > instances( &mem );

            const Policy oldPolicy = policy;
            policy = static_cast<Policy>( instances->policy );
            policyDidChange = policy != oldPolicy;

            // check for the primary instance health status
            if( now - instances->info[ 0 ].timestamp > KDSINGLEAPPLICATIONGUARD_TIMEOUT_SECONDS )
            {
                std::swap( instances->info[ 0 ], instances->info[ id ] );
                id = 0;
                instances->info[ id ].timestamp = now;
                emit q->becamePrimaryInstance();
                instances->info[ id ].command &= ~BecomePrimaryCommand;  // afterwards, reset the flag
            }

            if( instances->info[ id ].command & BecomePrimaryCommand )
            {
                // we became primary!
                instances->info[ 0 ] = instances->info[ id ];
                instances->info[ id ] = ProcessInfo();  // change our id to 0 and declare the old slot as free
                id = 0;
                instances->info[ id ].timestamp = now;
                emit q->becamePrimaryInstance();
            }

            if( instances->info[ id ].command & RaiseCommand )
            {
               // raise ourself!
               emit q->raiseRequested();
               instances->info[ id ].command &= ~RaiseCommand;  // afterwards, reset the flag
            }


            killOurSelf = instances->info[ id ].command & KillCommand;            // check for kill command
            shutDownOurSelf = instances->info[ id ].command & ShutDownCommand;    // check for shut down command
            instances->info[ id ].command &= ~( KillCommand | ShutDownCommand | BecomePrimaryCommand );  // reset both flags
            if( killOurSelf )
            {
                instances->info[ id ].command |= ExitedInstance;  // upon kill, we have to set the ExitedInstance flag
                id = -1;                                          // becauso our d'tor won't be called anymore
            }
        }

        if( killOurSelf )  // kill our self takes precedence
        {
            exitRequested = true;
            emit q->exitRequested();
        }
        else if( shutDownOurSelf )
            qApp->quit();
        else if( policyDidChange )
            emit q->policyChanged( policy );
    }
}

#include "moc_kdsingleapplicationguard.cpp"

#ifdef KDTOOLSCORE_UNITTESTS

#include <kdunittest/test.h>

#include "kdautopointer.h"

#include <iostream>

#include <QtCore/QTime>
#include <QtCore/QUuid>
#include <QtTest/QSignalSpy>

static void wait( int msec, QSignalSpy * spy=0, int expectedCount=INT_MAX )
{
    QTime t;
    t.start();
    while ( ( !spy || spy->count() < expectedCount ) && t.elapsed() < msec )
    {
        qApp->processEvents( QEventLoop::WaitForMoreEvents, qMax( 10, msec - t.elapsed() ) );
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

namespace {
    class ApplicationNameSaver {
        Q_DISABLE_COPY( ApplicationNameSaver )
        const QString oldname;
    public:
        explicit ApplicationNameSaver( const QString & name )
            : oldname( QCoreApplication::applicationName() )
        {
            QCoreApplication::setApplicationName( name );
        }
        ~ApplicationNameSaver() {
            QCoreApplication::setApplicationName( oldname );
        }
    };
}

KDAB_UNITTEST_SIMPLE( KDSingleApplicationGuard, "kdcoretools" ) {

    // set it to an unique name
    const ApplicationNameSaver saver( QUuid::createUuid().toString() );

    KDAutoPointer<KDSingleApplicationGuard> guard3;
    KDAutoPointer<QSignalSpy> spy3;
    KDAutoPointer<QSignalSpy> spy4;

    {
        KDSingleApplicationGuard guard1;
        assertEqual( guard1.policy(), KDSingleApplicationGuard::AutoKillOtherInstances );
        assertEqual( guard1.instances().count(), 1 );
        assertTrue( guard1.isPrimaryInstance() );

        guard1.setPolicy( KDSingleApplicationGuard::NoPolicy );
        assertEqual( guard1.policy(), KDSingleApplicationGuard::NoPolicy );

        QSignalSpy spy1( &guard1, SIGNAL(instanceStarted(KDSingleApplicationGuard::Instance)) );

        KDSingleApplicationGuard guard2;
        assertEqual( guard1.instances().count(), 2 );
        assertEqual( guard2.instances().count(), 2 );
        assertEqual( guard2.policy(), KDSingleApplicationGuard::NoPolicy );
        assertFalse( guard2.isPrimaryInstance() );

        wait( 1000, &spy1, 1 );

        assertEqual( spy1.count(), 1 );
        guard3.reset( new KDSingleApplicationGuard );
        spy3.reset( new QSignalSpy( guard3.get(), SIGNAL(becamePrimaryInstance()) ) );
        spy4.reset( new QSignalSpy( guard3.get(), SIGNAL(instanceExited(KDSingleApplicationGuard::Instance) ) ) );
        assertFalse( guard3->isPrimaryInstance() );
    }

    wait( 1000, spy3.get(), 1 );
    wait( 1000, spy4.get(), 1 );
    assertEqual( spy3->count(), 1 );
    assertEqual( guard3->instances().count(), 1 );
    assertTrue( guard3->isPrimaryInstance() );
    guard3.reset( new KDSingleApplicationGuard );

    assertEqual( guard3->instances().first().arguments(), qApp->arguments() );

    QSignalSpy spyStarted( guard3.get(), SIGNAL(instanceStarted(KDSingleApplicationGuard::Instance)) );
    QSignalSpy spyExited(  guard3.get(), SIGNAL(instanceExited(KDSingleApplicationGuard::Instance)) );

    {
        KDSingleApplicationGuard guard1;
        KDSingleApplicationGuard guard2;

        wait( 1000, &spyStarted, 2 );

        assertEqual( spyStarted.count(), 2 );
    }

    wait( 1000, &spyExited, 2 );
    assertEqual( spyExited.count(), 2 );

    spyStarted.clear();
    spyExited.clear();

    {
        // check arguments-too-long handling:
        QStringList args;
        for ( unsigned int i = 0, end = KDSINGLEAPPLICATIONGUARD_MAX_COMMAND_LINE/16 ; i != end ; ++i )
            args.push_back( QLatin1String( "0123456789ABCDEF" ) );
        KDSingleApplicationGuard guard3( args );

        wait( 1000, &spyStarted, 1 );

        const QVector<KDSingleApplicationGuard::Instance> instances = guard3.instances();
        assertEqual( instances.size(), 2 );

        assertTrue( instances[1].areArgumentsTruncated() );
    }
}

#endif // KDTOOLSCORE_UNITTESTS

#endif // QT_NO_SHAREDMEMORY
#endif // QT_VERSION >= 0x040400 || defined(DOXYGEN_RUN)
