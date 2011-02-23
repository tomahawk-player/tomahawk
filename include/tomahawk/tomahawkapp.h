#ifndef TOMAHAWKAPP_H
#define TOMAHAWKAPP_H

#define APP TomahawkApp::instance()

#include "headlesscheck.h"
#include "mac/tomahawkapp_mac.h" // for PlatforInterface

#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QDir>

#include "QxtHttpServerConnector"
#include "QxtHttpSessionManager"

#include "tomahawk/tomahawkplugin.h"
#include "typedefs.h"
#include "playlist.h"

#include "utils/tomahawkutils.h"

class ScriptResolver;
class AudioEngine;
class Database;
class SipHandler;
class TomahawkSettings;
class XMPPBot;

namespace Tomahawk
{
    class ShortcutHandler;
    namespace InfoSystem
    {
        class InfoSystem;
    }
}

#ifdef LIBLASTFM_FOUND
#include <lastfm/NetworkAccessManager>
#include "scrobbler.h"
#endif

#ifndef TOMAHAWK_HEADLESS
class TomahawkWindow;
#endif


// this also acts as a a container for important top-level objects
// that other parts of the app need to find
// (eg, library, pipeline, friends list)
class TomahawkApp : public TOMAHAWK_APPLICATION, public Tomahawk::PlatformInterface
{
Q_OBJECT

public:
    TomahawkApp( int& argc, char *argv[] );
    virtual ~TomahawkApp();

    static TomahawkApp* instance();

    SipHandler* sipHandler() { return m_sipHandler; }
    Tomahawk::InfoSystem::InfoSystem* infoSystem() { return m_infoSystem; }
    XMPPBot* xmppBot() { return m_xmppBot; }

#ifndef TOMAHAWK_HEADLESS
    AudioControls* audioControls();
    TomahawkWindow* mainWindow() const { return m_mainwindow; }
#endif

    void addScriptResolver( const QString& scriptPath );
    void removeScriptResolver( const QString& scriptPath );

    // PlatformInterface
    virtual void activate();
    virtual bool loadUrl( const QString& url );

private slots:
    void setupSIP();
    void messageReceived( const QString& );

private:
    void initLocalCollection();
    void loadPlugins();
    void registerMetaTypes();
    void startServent();
    void setupDatabase();
    void setupPipeline();
    void startHTTP();

    QList<Tomahawk::collection_ptr> m_collections;
    QList<TomahawkPlugin*> m_plugins;
    QList<ScriptResolver*> m_scriptResolvers;

    Database* m_database;
    AudioEngine* m_audioEngine;
    SipHandler* m_sipHandler;
    Servent* m_servent;
    XMPPBot* m_xmppBot;
    Tomahawk::ShortcutHandler* m_shortcutHandler;

#ifdef LIBLASTFM_FOUND
    Scrobbler* m_scrobbler;
#endif

#ifndef TOMAHAWK_HEADLESS
    TomahawkWindow* m_mainwindow;
#endif    

    bool m_headless;

    Tomahawk::InfoSystem::InfoSystem* m_infoSystem;

    QxtHttpServerConnector m_connector;
    QxtHttpSessionManager m_session;
};

#endif // TOMAHAWKAPP_H
