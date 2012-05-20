#include "kdlockedsharedmemorypointer.h"

#if QT_VERSION >= 0x040400 || defined( DOXYGEN_RUN )
#ifndef QT_NO_SHAREDMEMORY

namespace kdtools
{
}
using namespace kdtools;

KDLockedSharedMemoryPointerBase::KDLockedSharedMemoryPointerBase( QSharedMemory * m )
    : locker( m ),
      mem( m )
{

}

KDLockedSharedMemoryPointerBase::KDLockedSharedMemoryPointerBase( QSharedMemory & m )
    : locker( &m ),
      mem( &m )
{

}

KDLockedSharedMemoryPointerBase::~KDLockedSharedMemoryPointerBase() {}

void * KDLockedSharedMemoryPointerBase::get() {
    return mem ? mem->data() : 0 ;
}

const void * KDLockedSharedMemoryPointerBase::get() const {
    return mem ? mem->data() : 0 ;
}

size_t KDLockedSharedMemoryPointerBase::byteSize() const {
    return mem ? mem->size() : 0;
}

/*!
  \class KDLockedSharedMemoryPointer
  \ingroup core raii smartptr
  \brief Locking pointer for Qt shared memory segments
  \since_c 2.1

  (The exception safety of this class has not been evaluated yet.)

  KDLockedSharedMemoryPointer is a smart immutable pointer, which gives convenient and safe access to a QSharedMemory data segment.
  The content of a KDLockedSharedMemoryPointer cannot be changed during it's lifetime.

  You can use this class like a normal pointer to the shared memory segment and be sure it's locked while accessing it.
  \note You can only put simple types/structs/classes into it. structs and classes shall not contain any other pointers. See the
  documentation of QSharedMemory for details.
*/

/*!
  \fn KDLockedSharedMemoryPointer::KDLockedSharedMemoryPointer( QSharedMemory * mem )

  Constructor. Constructs a KDLockedSharedMemory pointer which points to the data segment of \a mem.
  The constructor locks \a mem. If the memory segment is already locked by another process, this constructor
  blocks until the lock is released.

  \post data() == mem->data() and the memory segment has been locked
*/

/*!
  \fn KDLockedSharedMemoryPointer::KDLockedSharedMemoryPointer( QSharedMemory & mem )

  \overload

  \post data() == mem.data() and the memory segment has been locked
*/

/*!
  \fn KDLockedSharedMemoryPointer::~KDLockedSharedMemoryPointer()

  Destructor. Unlocks the shared memory segment.

  \post The shared memory segment has been unlocked
*/

/*!
  \fn T * KDLockedSharedMemoryPointer::get()

  \returns a pointer to the contained object.
*/

/*!
  \fn const T * KDLockedSharedMemoryPointer::get() const

  \returns a const pointer to the contained object
  \overload
*/

/*!
  \fn T * KDLockedSharedMemoryPointer::data()

  Equivalent to get(), provided for consistency with Qt naming conventions.
*/

/*!
  \fn const T * KDLockedSharedMemoryPointer::data() const

  \overload
*/

/*!
  \fn T & KDLockedSharedMemoryPointer::operator*()

  Dereference operator. Returns \link get() *get()\endlink.
*/

/*!
  \fn const T & KDLockedSharedMemoryPointer::operator*() const

  Dereference operator. Returns \link get() *get()\endlink.
  \overload
*/

/*!
  \fn T * KDLockedSharedMemoryPointer::operator->()

  Member-by-pointer operator. Returns get().
*/

/*!
  \fn const T * KDLockedSharedMemoryPointer::operator->() const

  Member-by-pointer operator. Returns get().
  \overload
*/

/*!
  \class KDLockedSharedMemoryArray
  \ingroup core raii smartptr
  \brief Locking array pointer to Qt shared memory segments
  \since_c 2.1

  (The exception safety of this class has not been evaluated yet.)

  KDLockedSharedMemoryArray is a smart immutable pointer, which gives convenient and safe access to array data stored in a QSharedMemory
  data segment.
  The content of a KDLockedSharedMemoryArray cannot be changed during it's lifetime.

  You can use this class like a normal pointer to the shared memory segment and be sure it's locked while accessing it.
  \note You can only put arrays of simple types/structs/classes into it. structs and classes shall not contain any other pointers. See the
  documentation of QSharedMemory for details.

  \sa KDLockedSharedMemoryPointer
*/

/*!
  \fn KDLockedSharedMemoryArray::KDLockedSharedMemoryArray( QSharedMemory* mem )
  Constructor. Constructs a KDLockedSharedMemoryArray which points to the data segment of \a mem. The constructor locks \a mem. If the memory
  segment is already locked by another process, this constructor blocks until the lock is release.

  \post get() == mem->data() and the memory segment has been locked
*/

/*!
  \fn KDLockedSharedMemoryArray::KDLockedSharedMemoryArray( QSharedMemory& mem )
  \overload

  \post get() == mem->data() and the memory segment has been locked
*/


/*!
  \typedef KDLockedSharedMemoryArray::size_type
  Typedef for std::size_t. Provided for STL compatibility.
*/

/*!
  \typedef KDLockedSharedMemoryArray::difference_type
  Typedef for std::ptrdiff_t. Provided for STL compatibility.
*/

/*!
  \typedef KDLockedSharedMemoryArray::iterator
  Typedef for T*. Provided for STL compatibility.
  \since_t 2.2
*/

/*!
  \typedef KDLockedSharedMemoryArray::const_iterator
  Typedef for const T*. Provided for STL compatibility.
  \since_t 2.2
*/

/*!
  \typedef KDLockedSharedMemoryArray::reverse_iterator
  Typedef for std::reverse_iterator< \link KDLockedSharedMemoryArray::iterator iterator\endlink >. Provided for STL compatibility.
  \since_t 2.2
*/

/*!
  \typedef KDLockedSharedMemoryArray::const_reverse_iterator
  Typedef for std::reverse_iterator< \link KDLockedSharedMemoryArray::const_iterator const_iterator\endlink >. Provided for STL compatibility.
  \since_t 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::iterator KDLockedSharedMemoryArray::begin()
  Returns an \link KDLockedSharedMemoryArray::iterator iterator\endlink pointing to the first item of the array.
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::const_iterator KDLockedSharedMemoryArray::begin() const
  \overload
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::iterator KDLockedSharedMemoryArray::end()
  Returns an \link KDLockedSharedMemoryArray::iterator iterator\endlink pointing to the item after the last item of the array.
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::const_iterator KDLockedSharedMemoryArray::end() const
  \overload
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::reverse_iterator KDLockedSharedMemoryArray::rbegin()
  Returns an \link KDLockedSharedMemoryArray::reverse_iterator reverse_iterator\endlink pointing to the item after the last item of the array.
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::const_reverse_iterator KDLockedSharedMemoryArray::rbegin() const
  \overload
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::reverse_iterator KDLockedSharedMemoryArray::rend()
  Returns an \link KDLockedSharedMemoryArray::reverse_iterator reverse_iterator\endlink pointing to the first item of the array.
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::const_reverse_iterator KDLockedSharedMemoryArray::rend() const
  \overload
  \since_f 2.2
*/

/*!
  \fn KDLockedSharedMemoryArray::size_type KDLockedSharedMemoryArray::size() const
  Returns the size of this array. The size is calculated from the storage size of T and
  the size of the shared memory segment.
  \since_f 2.2
*/

/*!
  \fn T& KDLockedSharedMemoryArray::operator[]( difference_type n )
  Array access operator. Returns a reference to the item at index position \a n.
*/

/*!
  \fn const T& KDLockedSharedMemoryArray::operator[]( difference_type n ) const
  \overload
*/

/*!
 \fn T& KDLockedSharedMemoryArray::front()
 Returns a reference to the first item in the array. This is the same as operator[](0).
*/

/*!
 \fn const T& KDLockedSharedMemoryArray::front() const
 \overload
*/

/*!
 \fn T& KDLockedSharedMemoryArray::back()
 Returns a reference to the last item in the array. This is the same as operator[](size()-1).
 \since_f 2.2
*/

/*!
 \fn const T& KDLockedSharedMemoryArray::back() const
 \overload
 \since_f 2.2
*/


#ifdef eKDTOOLSCORE_UNITTESTS

#include <KDUnitTest/Test>

#include <QThread>
#include <QUuid>

namespace
{
    struct TestStruct
    {
        TestStruct( uint nn = 0 )
            : n( nn ),
              f( 0.0 ),
              c( '\0' ),
              b( false )
        {
        }
        uint n;
        double f;
        char c;
        bool b;
    };

    bool operator==( const TestStruct& lhs, const TestStruct& rhs )
    {
        return lhs.n == rhs.n && lhs.f == rhs.f && lhs.c == rhs.c && lhs.b == rhs.b;
    }

    class TestThread : public QThread
    {
    public:
        TestThread( const QString& key )
            : mem( key )
        {
            mem.attach();
        }

        void run()
        {
            while( true )
            {
                msleep( 100 );
                kdtools::KDLockedSharedMemoryPointer< TestStruct > p( &mem );
                if( !p->b )
                    continue;

                p->n = 5;
                p->f = 3.14;
                p->c = 'A';
                p->b = false;
                return;
            }
        }

        QSharedMemory mem;
    };

    bool isConst( TestStruct* )
    {
        return false;
    }

    bool isConst( const TestStruct* )
    {
        return true;
    }
}


KDAB_UNITTEST_SIMPLE( KDLockedSharedMemoryPointer, "kdcoretools" ) {

    const QString key = QUuid::createUuid();
    QSharedMemory mem( key );
    const bool created = mem.create( sizeof( TestStruct ) );
    assertTrue( created );
    if ( !created )
        return; // don't execute tests if shm coulnd't be created

    // On Windows, shared mem is only available in increments of page
    // size (4k), so don't fail if the segment is larger:
    const unsigned long mem_size = mem.size();
    assertGreaterOrEqual( mem_size, sizeof( TestStruct ) );

    {
        kdtools::KDLockedSharedMemoryPointer< TestStruct > p( &mem );
        assertTrue( p );
        *p = TestStruct();
        assertEqual( p->n, 0u );
        assertEqual( p->f, 0.0 );
        assertEqual( p->c, '\0' );
        assertFalse( p->b );
    }

    {
        TestThread thread( key );
        assertEqual( thread.mem.key().toStdString(), key.toStdString() );
        assertEqual( static_cast< unsigned long >( thread.mem.size() ), mem_size );
        thread.start();

        assertTrue( thread.isRunning() );
        thread.wait( 2000 );
        assertTrue( thread.isRunning() );

        {
            kdtools::KDLockedSharedMemoryPointer< TestStruct > p( &mem );
            p->b = true;
        }

        thread.wait( 2000 );
        assertFalse( thread.isRunning() );
    }

    {
        kdtools::KDLockedSharedMemoryPointer< TestStruct > p( &mem );
        assertEqual( p->n, 5u );
        assertEqual( p->f, 3.14 );
        assertEqual( p->c, 'A' );
        assertFalse( p->b );
    }

    {
        kdtools::KDLockedSharedMemoryPointer< TestStruct > p( mem );
        assertEqual( mem.data(), p.get() );
        assertEqual( p.get(), p.operator->() );
        assertEqual( p.get(), &(*p) );
        assertEqual( p.get(), p.data() );
        assertFalse( isConst( p.get() ) );
    }

    {
        const kdtools::KDLockedSharedMemoryPointer< TestStruct > p( &mem );
        assertEqual( mem.data(), p.get() );
        assertEqual( p.get(), p.operator->() );
        assertEqual( p.get(), &(*p) );
        assertEqual( p.get(), p.data() );
        assertTrue( isConst( p.get() ) );
    }

    {
        QSharedMemory mem2( key + key );
        const bool created2 = mem2.create( 16 * sizeof( TestStruct ) );
        assertTrue( created2 );
        if ( !created2 )
            return; // don't execute tests if shm coulnd't be created

        kdtools::KDLockedSharedMemoryArray<TestStruct> a( mem2 );
        assertTrue( a );
        assertEqual( a.get(), mem2.data() );
        assertEqual( &a[0], a.get() );

        a[1] = a[0];
        assertTrue( a[0] == a[1] );

        TestStruct ts;
        ts.n = 5;
        ts.f = 3.14;
        a[0] = ts;
        assertFalse( a[0] == a[1] );
        assertEqual( a.front().n, ts.n );
        assertEqual( a[0].f, ts.f );
        a[0].n = 10;
        assertEqual( a.front().n, 10u );
        ts = a[0];
        assertEqual( ts.n, 10u );

        std::vector< TestStruct > v;
        for( uint i = 0; i < a.size(); ++i )
            v.push_back( TestStruct( i ) );

        std::copy( v.begin(), v.end(), a.begin() );
        for( uint i = 0; i < a.size(); ++i )
            assertEqual( a[ i ].n, i );
        assertEqual( a.front().n, 0u );
        assertEqual( a.back().n, a.size() - 1 );

        std::copy( v.begin(), v.end(), a.rbegin() );
        for( uint i = 0; i < a.size(); ++i )
            assertEqual( a[ i ].n, a.size() - 1 - i );
        assertEqual( a.front().n, a.size() - 1 );
        assertEqual( a.back().n, 0u );
     }

}
#endif // KDTOOLSCORE_UNITTESTS
#endif // QT_NO_SHAREDMEMORY
#endif // QT_VERSION >= 0x040400 || defined( DOXYGEN_RUN )
