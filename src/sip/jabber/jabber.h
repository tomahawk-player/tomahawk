#ifndef JABBER_H
#define JABBER_H

#include "sip/SipPlugin.h"
#include "jabber_p.h"

#include "../sipdllmacro.h"

#define MYNAME "SIPJABBER"

class SIPDLLEXPORT JabberPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    JabberPlugin()
        : p( 0 )
    {}

    virtual ~JabberPlugin() { delete p; }

    //FIXME: Make this more correct
    virtual bool isValid() { return true; }
    virtual const QString name();

    void setProxy( QNetworkProxy* proxy );

public slots:
    virtual bool connectPlugin( bool startup );

    void disconnectPlugin()
    {
        if ( p )
            p->disconnect();
    }

    void sendMsg( const QString& to, const QString& msg )
    {
        if ( p )
            p->sendMsg( to, msg );
    }

    void broadcastMsg( const QString &msg )
    {
        if ( p )
            p->broadcastMsg( msg );
    }

    void addContact( const QString &jid, const QString& msg = QString() )
    {
        if ( p )
            p->addContact( jid, msg );
    }

private slots:
    void onAuthError( int, const QString& );

private:
    Jabber_p* p;
};

#endif
