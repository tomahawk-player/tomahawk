#include "pimpl_ptr.h"

/*!
  \class pimpl_ptr:
  \ingroup core smartptr
  \brief Owning pointer for private implementations
  \since_c 2.1

  (The exception safety of this class has not been evaluated yet.)

  pimpl_ptr is a smart immutable pointer, which owns the contained object. Unlike other smart pointers,
  it creates a standard constructed object when instanciated via the 
  \link pimpl_ptr() standard constructor\endlink.
  Additionally, pimpl_ptr respects constness of the pointer object and returns \c const \c T* for
  a const pimpl_ptr object.

  The content of a pimpl_ptr cannot be changed during it's lifetime.

  \section general-use General Use

  The general use case of pimpl_ptr is the "Pimpl Idiom", i.e. hiding the private implementation of a class
  from the user's compiler which see \c MyClass as

  \code
  class MyClass
  {
  public:
      MyClass();
      ~MyClass();

      // public class API
      int value() const;

  private:
      class Private; // defined later
      kdtools::pimpl_ptr< Private > d;
  };
  \endcode

  but not the private parts of it. These can only be seen (and accessed) by the code knowing \c MyClass::Private:

  \code
  class MyClass::Private
  {
  public:
      int value;
  };

  MyClass::MyClass()
  {
      // d was automatically filled with new Private
      d->value = 42;
  }

  MyClass::~MyClass()
  {
      // the content of d gets deleted automatically
  }

  int MyClass::value() const
  {
      // access the private part:
      // since MyClass::value() is const, the returned pointee is const, too
      return d->value;
  }
  \endcode

*/

/*!
  \fn pimpl_ptr::pimpl_ptr()

  Default constructor. Constructs a pimpl_tr that contains (owns) a standard constructed
  instance of \c T.

  \post \c *this owns a new object.
*/

/*!
  \fn pimpl_ptr::pimpl_ptr( T * t )

  Constructor. Constructs a pimpl_ptr that contains (owns) \a t.

  \post get() == obj
*/

/*!
  \fn pimpl_ptr::~pimpl_ptr()

  Destructor.

  \post The object previously owned by \c *this has been deleted.
*/

/*!
  \fn const T * pimpl_ptr::get() const

  \returns a const pointer to the contained (owned) object.
  \overload
*/

/*!
  \fn T * pimpl_ptr::get()

  \returns a pointer to the contained (owned) object.
*/

/*!
  \fn const T & pimpl_ptr::operator*() const

  Dereference operator. Returns \link get() *get()\endlink.
  \overload
*/

/*!
  \fn T & pimpl_ptr::operator*()

  Dereference operator. Returns \link get() *get()\endlink.
*/

/*!
  \fn const T * pimpl_ptr::operator->() const

  Member-by-pointer operator. Returns get().
  \overload
*/

/*!
  \fn T * pimpl_ptr::operator->() 

  Member-by-pointer operator. Returns get().
*/

#ifdef KDTOOLSCORE_UNITTESTS

#include <kdunittest/test.h>

#include <QObject>
#include <QPointer>

namespace
{
    struct ConstTester
    {
        bool isConst()
        {
            return false;
        }

        bool isConst() const
        {
            return true;
        }
    };
}

KDAB_UNITTEST_SIMPLE( pimpl_ptr, "kdcoretools" ) {

    {
        kdtools::pimpl_ptr< QObject > p;
        assertNotNull( p.get() );
        assertNull( p->parent() );
    }


    {   
        QPointer< QObject > o;
        {
            kdtools::pimpl_ptr< QObject > qobject( new QObject );
            o = qobject.get();
            assertEqual( o, qobject.operator->() );
            assertEqual( o, &(qobject.operator*()) );
        }
        assertNull( o );
    }

    {   
        const kdtools::pimpl_ptr< QObject > qobject( new QObject );
        const QObject* o = qobject.get();
        assertEqual( o, qobject.operator->() );
        assertEqual( o, &(qobject.operator*()) );
    }

    {
        kdtools::pimpl_ptr< QObject > o1;
        assertTrue( o1 );
        kdtools::pimpl_ptr< QObject > o2( 0 );
        assertFalse( o2 );
    }

    {
        const kdtools::pimpl_ptr< ConstTester > o1;
        kdtools::pimpl_ptr< ConstTester > o2;
        assertTrue( o1->isConst() );
        assertFalse( o2->isConst() );
        assertTrue( (*o1).isConst() );
        assertFalse( (*o2).isConst() );
        assertTrue( o1.get()->isConst() );
        assertFalse( o2.get()->isConst() );
    }
}

#endif // KDTOOLSCORE_UNITTESTS
