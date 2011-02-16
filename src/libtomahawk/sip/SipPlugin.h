#ifndef SIPPLUGIN_H
#define SIPPLUGIN_H

#include <QObject>
#include <QString>
#include <QMenu>

#include "dllmacro.h"

class DLLEXPORT SipPlugin : public QObject
{
    Q_OBJECT

public:
    enum SipErrorCode { AuthError, ConnectionError }; // Placeholder for errors, to be defined

    virtual ~SipPlugin() {}
    
    virtual bool isValid() = 0;
    virtual const QString name() = 0;
    virtual const QString accountName() = 0;
    virtual QMenu *menu();

public slots:
    virtual bool connectPlugin( bool startup = false ) = 0;
    virtual void disconnectPlugin() = 0;

    virtual void addContact( const QString &jid, const QString& msg = QString() ) = 0;
    virtual void sendMsg( const QString& to, const QString& msg ) = 0;

signals:
    void error( int, const QString& );
    void connected();
    void disconnected();

    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void msgReceived( const QString& from, const QString& msg );
};

Q_DECLARE_INTERFACE( SipPlugin, "tomahawk.Sip/1.0" )

#endif
