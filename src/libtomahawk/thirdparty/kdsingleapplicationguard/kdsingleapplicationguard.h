#ifndef __KDTOOLSCORE_KDSINGLEAPPLICATIONGUARD_H__
#define __KDTOOLSCORE_KDSINGLEAPPLICATIONGUARD_H__

#include <QtCore/QObject>

#ifndef QT_NO_SHAREDMEMORY

#include <QtCore/QStringList>
#include <QtCore/QMetaType>

#include "pimpl_ptr.h"
#include "DllMacro.h"

#include <algorithm>

template <typename T> class QVector;
class QCoreApplication;

class DLLEXPORT KDSingleApplicationGuard : public QObject
{
    Q_OBJECT
    Q_ENUMS( Policy )
    Q_PROPERTY( bool operational READ isOperational )
    Q_PROPERTY( bool exitRequested READ isExitRequested )
    Q_PROPERTY( bool primaryInstance READ isPrimaryInstance NOTIFY becamePrimaryInstance )
    Q_PROPERTY( Policy policy READ policy WRITE setPolicy NOTIFY policyChanged )
public:
    enum Policy
    {
        NoPolicy = 0,
        AutoKillOtherInstances = 1
    };

    explicit KDSingleApplicationGuard( QObject * parent=0 );
    explicit KDSingleApplicationGuard( Policy policy, QObject * parent=0 );
    explicit KDSingleApplicationGuard( const QStringList & arguments, QObject * parent=0 );
    explicit KDSingleApplicationGuard( const QStringList & arguments, Policy policy, QObject * parent=0 );
    ~KDSingleApplicationGuard();

    bool isOperational() const;

    bool isExitRequested() const;

    bool isPrimaryInstance() const;

    Policy policy() const;
    void setPolicy( Policy policy );

    class Instance;

    QVector<Instance> instances() const;

Q_SIGNALS:
    void instanceStarted( const KDSingleApplicationGuard::Instance & instance );
    void instanceExited( const KDSingleApplicationGuard::Instance & instance );
    void exitRequested();
    void raiseRequested();
    void becamePrimaryInstance();
    void becameSecondaryInstance();
    void policyChanged( KDSingleApplicationGuard::Policy policy );

public Q_SLOTS:
    void shutdownOtherInstances();
    void killOtherInstances();

protected:
    /*! \reimp */ bool event( QEvent * event );

private:
#ifndef Q_WS_WIN
    static void SIGINT_handler( int );
#endif

private:
    friend struct ProcessInfo;

    class Private;
    kdtools::pimpl_ptr< Private > d;
};

class DLLEXPORT KDSingleApplicationGuard::Instance {
    friend class ::KDSingleApplicationGuard;
    friend class ::KDSingleApplicationGuard::Private;
    Instance( const QStringList &, bool, qint64 );
public:
    Instance();
    Instance( const Instance & other );
    ~Instance();

    void swap( Instance & other ) {
        std::swap( d, other.d );
    }

    Instance & operator=( Instance other ) {
        swap( other );
        return *this;
    }

    bool isNull() const { return !d; }
    bool isValid() const;

    bool areArgumentsTruncated() const;

    const QStringList & arguments() const;
    qint64 pid() const;

    void shutdown();
    void kill();
    void raise();

private:
    class Private;
    Private * d;
};

namespace std {
    template <>
    inline void swap( KDSingleApplicationGuard::Instance & lhs,
                      KDSingleApplicationGuard::Instance & rhs )
    {
        lhs.swap( rhs );
    }
} // namespace std

QT_BEGIN_NAMESPACE

template <>
inline void qSwap( KDSingleApplicationGuard::Instance & lhs,
                   KDSingleApplicationGuard::Instance & rhs )
{
    lhs.swap( rhs );
}
Q_DECLARE_METATYPE( KDSingleApplicationGuard::Instance )
Q_DECLARE_TYPEINFO( KDSingleApplicationGuard::Instance, Q_MOVABLE_TYPE );

QT_END_NAMESPACE


#endif // QT_NO_SHAREDMEMORY

#endif /* __KDTOOLSCORE_KDSINGLEAPPLICATIONGUARD_H__ */
