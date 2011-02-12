#ifndef ZEROCONF_H
#define ZEROCONF_H

#include "sip/SipPlugin.h"
#include "tomahawkoauthtwitter.h"
#include <qtweetuser.h>
#include <qtweetnetbase.h>

#include "../sipdllmacro.h"

class SIPDLLEXPORT TwitterPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    TwitterPlugin();

    virtual ~TwitterPlugin() {}
    
    virtual bool isValid();

public slots:
    virtual bool connect( bool startup );

    void disconnect()
    {
    }

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
    void connectAuthVerifyReply( const QTweetUser &user );
    void connectAuthVerifyError( QTweetNetBase::ErrorCode errorCode, const QString& errorMsg );

private:
    TomahawkOAuthTwitter *m_twitterAuth;
    bool m_isAuthed;
};

#endif
