#ifndef TOMAHAWKAPP_H
#define TOMAHAWKAPP_H

#define APP TomahawkApp::instance()

#define RESPATH ":/data/"

#include "headlesscheck.h"

#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QDir>

#include "QxtHttpServerConnector"
#include "QxtHttpSessionManager"

#include "tomahawk/functimeout.h"
#include "tomahawk/typedefs.h"
#include "tomahawk/tomahawkplugin.h"
#include "tomahawk/playlist.h"
#include "tomahawk/pipeline.h"

#include "utils/tomahawkutils.h"

#include "sourcelist.h"
#include "servent.h"

class AudioEngine;
class Database;
class SipHandler;
class TomahawkZeroconf;
class TomahawkSettings;
class XMPPBot;

namespace Tomahawk
{
    namespace InfoSystem
    {
        class InfoSystem;
    }
}

#ifndef NO_LIBLASTFM
#include <lastfm/NetworkAccessManager>
#include "scrobbler.h"
#endif

#ifndef TOMAHAWK_HEADLESS
class TomahawkWindow;
class PlaylistManager;
#include <QStackedWidget>
#endif


// this also acts as a a container for important top-level objects
// that other parts of the app need to find
// (eg, library, pipeline, friends list)
class TomahawkApp : public TOMAHAWK_APPLICATION
{
Q_OBJECT

public:
    TomahawkApp( int& argc, char *argv[] );
    virtual ~TomahawkApp();

    static TomahawkApp* instance();

    Tomahawk::Pipeline* pipeline() { return &m_pipeline; }
    AudioEngine* audioEngine() { return m_audioEngine; }
    Database* database() { return m_db; }
    SourceList& sourcelist() { return m_sources; }
    Servent& servent() { return m_servent; }
    SipHandler* sipHandler() { return m_sipHandler; }
    QNetworkAccessManager* nam() { return m_nam; }
    QNetworkProxy* proxy() { return m_proxy; }
    Tomahawk::InfoSystem::InfoSystem* infoSystem() { return m_infoSystem; }
    XMPPBot* xmppBot() { return m_xmppBot; }
    const QString& nodeID() const;

#ifndef TOMAHAWK_HEADLESS
    AudioControls* audioControls();
    PlaylistManager* playlistManager();
    TomahawkWindow* mainWindow() const { return m_mainwindow; }
#endif

    void registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac );
    QSharedPointer<QIODevice> localFileIODeviceFactory( const Tomahawk::result_ptr& result );
    QSharedPointer<QIODevice> httpIODeviceFactory( const Tomahawk::result_ptr& result );

    TomahawkSettings* settings() { return m_settings; }

signals:
    void settingsChanged();
    
public slots:
    QSharedPointer<QIODevice> getIODeviceForUrl( const Tomahawk::result_ptr& result );

private slots:
    void lanHostFound( const QString&, int, const QString&, const QString& );

private:
    void initLocalCollection();
    void loadPlugins();
    void registerMetaTypes();
    void startServent();
    void setupDatabase();
    void setupSIP();
    void setupPipeline();
    void startHTTP();

    QList<Tomahawk::collection_ptr> m_collections;
    QList<TomahawkPlugin*> m_plugins;

    Tomahawk::Pipeline m_pipeline;
    AudioEngine* m_audioEngine;
    Database* m_db;
    Servent m_servent;
    SourceList m_sources;
    TomahawkZeroconf* m_zeroconf;
    SipHandler* m_sipHandler;
    XMPPBot* m_xmppBot;

#ifndef NO_LIBLASTFM
    Scrobbler* m_scrobbler;
#endif

#ifndef TOMAHAWK_HEADLESS
    TomahawkWindow* m_mainwindow;
#endif    

    QMap< QString,boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> > m_iofactories;

    bool m_headless;
    TomahawkSettings* m_settings;

    QNetworkAccessManager* m_nam;
    QNetworkProxy* m_proxy;

    Tomahawk::InfoSystem::InfoSystem* m_infoSystem;

    QxtHttpServerConnector m_connector;
    QxtHttpSessionManager m_session;
};

#endif // TOMAHAWKAPP_H
