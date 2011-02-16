#ifndef SIPHANDLER_H
#define SIPHANDLER_H

#include "sip/SipPlugin.h"

#include <QDebug>
#include <QObject>

class SipHandler : public QObject
{
    Q_OBJECT

public:
//    static SipHandler* instance() { return s_instance ? s_instance : new SipHandler(); }

    SipHandler( QObject* parent );
    ~SipHandler();

    QList< SipPlugin* > plugins() const;

public slots:
    void addContact( const QString& id ) { qDebug() << Q_FUNC_INFO << id; }

    void connectPlugins( bool startup = false, const QString &pluginName = QString() );
    void disconnectPlugins( const QString &pluginName = QString() );
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

    void onSettingsChanged();

private:
    QStringList findPlugins();
    void loadPlugins( const QStringList& paths );
    void loadPlugin( QObject* plugin );

    QList< SipPlugin* > m_plugins;
    bool m_connected;
};

#endif
