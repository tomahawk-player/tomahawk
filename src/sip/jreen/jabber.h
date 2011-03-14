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
    JabberPlugin();
    virtual ~JabberPlugin();

    //FIXME: Make this more correct
    virtual bool isValid() { return true; }
    virtual const QString name();
    virtual const QString friendlyName();
    virtual const QString accountName();
    virtual QMenu* menu();

    void setProxy( QNetworkProxy* proxy );

public slots:
    virtual bool connectPlugin( bool startup );
    void disconnectPlugin();
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString &msg );
    void addContact( const QString &jid, const QString& msg = QString() );

private slots:
    void showAddFriendDialog();
    void onConnected();
    void onDisconnected();

private:
    Jabber_p* p;
    QMenu* m_menu;
    QAction* m_addFriendAction;
};

#endif
