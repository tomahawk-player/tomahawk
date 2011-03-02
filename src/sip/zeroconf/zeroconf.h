#ifndef ZEROCONF_H
#define ZEROCONF_H

#include "sip/SipPlugin.h"
#include "tomahawkzeroconf.h"

#include "../sipdllmacro.h"

#define MYNAME "SIPZEROCONF"

class SIPDLLEXPORT ZeroconfPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    ZeroconfPlugin()
        : m_zeroconf( 0 )
        , m_isOnline( false )
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual ~ZeroconfPlugin()
    {
        qDebug() << Q_FUNC_INFO;
    }
    
    virtual bool isValid() { return true; }
    virtual const QString name();
    virtual const QString friendlyName();
    virtual const QString accountName();

public slots:
    virtual bool connectPlugin( bool startup );

    void disconnectPlugin();

    void sendMsg( const QString& to, const QString& msg )
    {
    }

    void broadcastMsg( const QString &msg )
    {
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
    }

private slots:
    void lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid );

private:
    TomahawkZeroconf* m_zeroconf;
    bool m_isOnline;
};

#endif
