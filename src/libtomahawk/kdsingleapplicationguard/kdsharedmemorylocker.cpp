#include "kdsharedmemorylocker.h"

#if QT_VERSION >= 0x040400 || defined( DOXYGEN_RUN )

#include <QSharedMemory>

using namespace kdtools;

/*!
  \class KDSharedMemoryLocker
  \ingroup raii core
  \brief Exception-safe and convenient wrapper around QSharedMemory::lock()
*/

/**
 * Constructor. Locks the shared memory segment \a mem.
 * If another process has locking the segment, this constructor blocks
 * until the lock is released. The memory segments needs to be properly created or attached.
 */
KDSharedMemoryLocker::KDSharedMemoryLocker( QSharedMemory* mem )
    : mem( mem )
{
    mem->lock();
}

/**
 * Destructor. Unlocks the shared memory segment associated with this
 * KDSharedMemoryLocker.
 */
KDSharedMemoryLocker::~KDSharedMemoryLocker()
{
    mem->unlock();
}

#ifdef KDAB_EVAL
#include KDAB_EVAL
static const EvalDialogChecker evalChecker( "KD Tools", false );
#endif

#endif
