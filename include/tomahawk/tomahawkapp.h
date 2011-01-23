#ifndef TOMAHAWKAPP_H
#define TOMAHAWKAPP_H

#define APP TomahawkApp::instance()

#include "headlesscheck.h"

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

class AudioEngine;
class Database;
class SipHandler;
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

    SipHandler* sipHandler() { return m_sipHandler; }
    Tomahawk::InfoSystem::InfoSystem* infoSystem() { return m_infoSystem; }
    XMPPBot* xmppBot() { return m_xmppBot; }

#ifndef TOMAHAWK_HEADLESS
    AudioControls* audioControls();
    TomahawkWindow* mainWindow() const { return m_mainwindow; }
#endif

signals:
    void settingsChanged();
    
private slots:
    void setupSIP();

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

    Database* m_database;
    AudioEngine* m_audioEngine;
    SipHandler* m_sipHandler;
    Servent* m_servent;
    XMPPBot* m_xmppBot;

#ifndef NO_LIBLASTFM
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
