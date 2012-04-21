#ifndef __KDTOOLSCORE_KDSINGLEAPPLICATIONGUARD_H__
#define __KDTOOLSCORE_KDSINGLEAPPLICATIONGUARD_H__

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "pimpl_ptr.h"
#include "DllMacro.h"

class QCoreApplication;

#ifndef Q_WS_WIN
void SIGINT_handler( int sig );
#endif

class DLLEXPORT KDSingleApplicationGuard : public QObject
{
    Q_OBJECT
#ifndef Q_WS_WIN
    friend void ::SIGINT_handler( int );
#endif

public:
    enum Policy
    {
        NoPolicy = 0,
        AutoKillOtherInstances = 1
    };

    Q_PROPERTY( bool primaryInstance READ isPrimaryInstance NOTIFY becamePrimaryInstance )
    Q_PROPERTY( Policy policy READ policy WRITE setPolicy NOTIFY policyChanged )

    explicit KDSingleApplicationGuard( QCoreApplication* parent, Policy policy = AutoKillOtherInstances );
    ~KDSingleApplicationGuard();

    bool isPrimaryInstance() const;
    
    Policy policy() const;
    void setPolicy( Policy policy );

    struct Instance 
    {
        Instance( const QStringList& arguments = QStringList(), qint64 pid = -1 );

        QStringList arguments;
        qint64 pid;
    };

    QList< Instance > instances() const;

Q_SIGNALS:
    void instanceStarted( KDSingleApplicationGuard::Instance instance );
    void instanceExited( KDSingleApplicationGuard::Instance instance );
    void becamePrimaryInstance();
    void policyChanged();

public Q_SLOTS:
    void shutdownOtherInstances();
    void killOtherInstances();

protected:
    void timerEvent( QTimerEvent* event );

private:
    class Private;
    kdtools::pimpl_ptr< Private > d;
};

#if QT_VERSION < 0x040400
#ifdef Q_CC_GNU
#warning "Can't use KDSingleApplicationGuard with Qt versions prior to 4.4"
#endif
#endif

#endif
