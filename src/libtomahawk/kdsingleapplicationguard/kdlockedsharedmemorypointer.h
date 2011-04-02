#ifndef __KDTOOLS__CORE__KDLOCKEDSHAREDMEMORYPOINTER_H__
#define __KDTOOLS__CORE__KDLOCKEDSHAREDMEMORYPOINTER_H__

#include <QtCore/QtGlobal>

#if QT_VERSION >= 0x040400 || defined( DOXYGEN_RUN )
#ifndef QT_NO_SHAREDMEMORY

#include "kdsharedmemorylocker.h"
#include <QtCore/QSharedMemory>

#include <cassert>

#ifndef DOXYGEN_RUN
namespace kdtools {
#endif

class KDLockedSharedMemoryPointerBase {
protected:
    explicit KDLockedSharedMemoryPointerBase( QSharedMemory * mem );
    explicit KDLockedSharedMemoryPointerBase( QSharedMemory & mem );
    ~KDLockedSharedMemoryPointerBase();

    // PENDING(marc) do we really want const propagation here? I
    // usually declare all my RAII objects const...
    void * get();
    const void * get() const;

    KDAB_IMPLEMENT_SAFE_BOOL_OPERATOR( get() )

    size_t byteSize() const;

private:
    KDSharedMemoryLocker locker;
    QSharedMemory * const mem;
};

template< typename T>
class MAKEINCLUDES_EXPORT KDLockedSharedMemoryPointer : KDLockedSharedMemoryPointerBase {
    KDAB_DISABLE_COPY( KDLockedSharedMemoryPointer );
public:
    explicit KDLockedSharedMemoryPointer( QSharedMemory * m )
        : KDLockedSharedMemoryPointerBase( m ) {}
    explicit KDLockedSharedMemoryPointer( QSharedMemory & m )
        : KDLockedSharedMemoryPointerBase( m ) {}

    T * get() { return static_cast<T*>( KDLockedSharedMemoryPointerBase::get() ); }
    const T * get() const { return static_cast<const T*>( KDLockedSharedMemoryPointerBase::get() ); }

    T * data() { return static_cast<T*>( get() ); }
    const T * data() const { return static_cast<const T*>( get() ); }

    T & operator*() { assert( get() ); return *get(); }
    const T & operator*() const { assert( get() ); return *get(); }

    T * operator->() { return get(); }
    const T * operator->() const { return get(); }

    KDAB_USING_SAFE_BOOL_OPERATOR( KDLockedSharedMemoryPointerBase )
};

template <typename T>
class MAKEINCLUDES_EXPORT KDLockedSharedMemoryArray : KDLockedSharedMemoryPointerBase {
    KDAB_DISABLE_COPY( KDLockedSharedMemoryArray );
public:
    explicit KDLockedSharedMemoryArray( QSharedMemory * m )
        : KDLockedSharedMemoryPointerBase( m ) {}
    explicit KDLockedSharedMemoryArray( QSharedMemory & m )
        : KDLockedSharedMemoryPointerBase( m ) {}

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;
    typedef std::reverse_iterator< iterator > reverse_iterator;
    
    iterator begin() { return get(); }
    const_iterator begin() const { return get(); }

    iterator end() { return begin() + size(); }
    const_iterator end() const { return begin() + size(); }

    reverse_iterator rbegin() { return reverse_iterator( end() ); }
    const_reverse_iterator rbegin() const { return reverse_iterator( end() ); }

    reverse_iterator rend() { return reverse_iterator( begin() ); }
    const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }

    size_type size() const { return byteSize() / sizeof( T ); }

    T * get() { return static_cast<T*>( KDLockedSharedMemoryPointerBase::get() ); }
    const T * get() const { return static_cast<const T*>( KDLockedSharedMemoryPointerBase::get() ); }

    T & operator[]( difference_type n ) { assert( get() ); return *(get()+n); }
    const T & operator[]( difference_type n ) const { assert( get() ); return *(get()+n); }

    T & front() { assert( get() ); return *get(); }
    const T & front() const { assert( get() ); return *get(); }

    T & back() { assert( get() ); return *( get() + size() - 1 ); }
    const T & back() const { assert( get() ); return *( get() + size() - 1 ); }

    KDAB_USING_SAFE_BOOL_OPERATOR( KDLockedSharedMemoryPointerBase )
};

#ifndef DOXYGEN_RUN
}
#endif

#endif /* QT_NO_SHAREDMEMORY */

#endif /* QT_VERSION >= 0x040400 || defined( DOXYGEN_RUN ) */

#endif /* __KDTOOLS__CORE__KDLOCKEDSHAREDMEMORYPOINTER_H__ */
