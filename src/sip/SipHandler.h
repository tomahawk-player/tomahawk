#ifndef SIPHANDLER_H
#define SIPHANDLER_H

#include <QDebug>
#include <QObject>

class SipPlugin;

class SipHandler : public QObject
{
    Q_OBJECT

public:
//    static SipHandler* instance() { return s_instance ? s_instance : new SipHandler(); }

    SipHandler( QObject* parent );
    ~SipHandler();

public slots:
    void addContact( const QString& id ) { qDebug() << Q_FUNC_INFO << id; }

    void connect();
    void disconnect();
    void toggleConnect();

signals:
    void connected();
    void disconnected();
    void authError();

private slots:
    void onMessage( const QString&, const QString& );
    void onPeerOffline( const QString& );
    void onPeerOnline( const QString& );
    void onError( int code, const QString& msg );

private:
    QStringList findPlugins();
    void loadPlugins( const QStringList& paths );
    void loadPlugin( QObject* plugin );

    QList< SipPlugin* > m_plugins;
    bool m_connected;
};

#endif
