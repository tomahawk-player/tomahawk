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

class Database;
class Jabber;
class TomahawkZeroconf;
class TomahawkSettings;

#ifndef TOMAHAWK_HEADLESS
class AudioEngine;
class TomahawkWindow;
class PlaylistView;

#ifndef NO_LIBLASTFM
#include <lastfm/NetworkAccessManager>
#include "scrobbler.h"
#endif

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
    Database* database() { return m_db; }
    SourceList& sourcelist() { return m_sources; }
    Servent& servent() { return m_servent; }
    QNetworkAccessManager* nam() { return m_nam; }
    const QString& nodeID() const;

#ifndef TOMAHAWK_HEADLESS
    AudioControls* audioControls();
    PlaylistView* playlistView();
    AudioEngine* audioEngine() { return m_audioEngine; }
#endif

    void registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac );
    QSharedPointer<QIODevice> localFileIODeviceFactory( const Tomahawk::result_ptr& result );
    QSharedPointer<QIODevice> httpIODeviceFactory( const Tomahawk::result_ptr& result );

    TomahawkSettings* settings() { return m_settings; }

signals:
    void settingsChanged();
    
public slots:
    QSharedPointer<QIODevice> getIODeviceForUrl( const Tomahawk::result_ptr& result );
    void reconnectJabber();

private slots:
    void jabberMessage( const QString&, const QString& );
    void jabberPeerOffline( const QString& );
    void jabberPeerOnline( const QString& );
    void jabberAuthError( int code, const QString& msg );
    void jabberDisconnected();
    void jabberConnected();

    void lanHostFound( const QString&, int, const QString&, const QString& );

private:
    void initLocalCollection();
    void loadPlugins();
    void registerMetaTypes();
    void startServent();
    void setupDatabase();
    void setupJabber();
    void setupPipeline();
    void startHTTP();

    QList<Tomahawk::collection_ptr> m_collections;
    QList<TomahawkPlugin*> m_plugins;

    Tomahawk::Pipeline m_pipeline;
    Database* m_db;
    Servent m_servent;
    SourceList m_sources;
    TomahawkZeroconf* m_zeroconf;
    QSharedPointer<Jabber> m_jabber;

#ifndef TOMAHAWK_HEADLESS
    TomahawkWindow* m_mainwindow;
    AudioEngine* m_audioEngine;
#ifndef NO_LIBLASTFM
    Scrobbler* m_scrobbler;
#endif
#endif    

    QMap< QString,boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> > m_iofactories;

    bool m_headless;
    TomahawkSettings* m_settings;

    QNetworkAccessManager* m_nam;

    QxtHttpServerConnector m_connector;
    QxtHttpSessionManager m_session;
};

#endif // TOMAHAWKAPP_H
