#ifndef SIPPLUGIN_H
#define SIPPLUGIN_H

#include <QObject>
#include <QString>

class SipPlugin : public QObject
{
    Q_OBJECT

public:
    enum SipErrorCode { AuthError, ConnectionError }; // Placeholder for errors, to be defined

    virtual ~SipPlugin() {}

public slots:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;

    virtual void addContact( const QString &jid, const QString& msg = QString() ) = 0;
    virtual void sendMsg( const QString& to, const QString& msg ) = 0;

signals:
    void error( int, const QString& );
    void connected();
    void disconnected();

    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void msgReceived( const QString&, const QString& );
};

Q_DECLARE_INTERFACE( SipPlugin, "tomahawk.Sip/1.0" )

#endif
