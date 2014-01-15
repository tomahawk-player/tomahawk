/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2014,      Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOMAHAWKAPP_H
#define TOMAHAWKAPP_H

#define APP TomahawkApp::instance()

#include "mac/TomahawkApp_Mac.h" // for PlatforInterface
#include "Typedefs.h"
#include "libtomahawk-playdarapi/PlaydarApi.h"
#include "utils/TomahawkUtils.h"
#include "thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.h"

#include "HeadlessCheck.h"
#include "config.h"

#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QDir>
#include <QPersistentModelIndex>
#include <QPointer>

class AudioEngine;
class ScanManager;
class Servent;
class SipHandler;
class TomahawkSettings;
class AudioControls;
class SplashWidget;

namespace Tomahawk
{
    class ShortcutHandler;
    class Database;
    namespace InfoSystem
    {
        class InfoSystem;
    }

    namespace Accounts
    {
        class AccountManager;
    }
}

#ifdef LIBLASTFM_FOUND
#include <lastfm/NetworkAccessManager.h>
#include "Scrobbler.h"
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

    void init();
    static TomahawkApp* instance();

#ifndef ENABLE_HEADLESS
    AudioControls* audioControls();
    TomahawkWindow* mainWindow() const;
#endif

    // PlatformInterface
    virtual bool loadUrl( const QString& url );

    // reimplemented from QApplication/QCoreApplication
    virtual bool notify( QObject* receiver, QEvent* e );

signals:
    void tomahawkLoaded();

public slots:
    virtual void activate();
    void instanceStarted( KDSingleApplicationGuard::Instance );

private slots:
    void playlistRemoved( const Tomahawk::playlist_ptr& playlist );
    void initServent();
    void initSIP();
    void initHTTP();
    void initFactoriesForAccountManager();
    void initEnergyEventHandler();

    void spotifyApiCheckFinished();
    void onInfoSystemReady();

    void onSchemaUpdateStarted();
    void onSchemaUpdateStatus( const QString& status );
    void onSchemaUpdateDone();

    void startSplashWidget( const QString& initialMessage = QString() );
    void updateSplashWidgetMessage( const QString& message );
    void killSplashWidget();

    void ipDetectionFailed( QNetworkReply::NetworkError error, QString errorString );

private:
    void registerMetaTypes();

    void printHelp();

    // Start-up order: database, collection, pipeline, servent, http
    void initDatabase();
    void initLocalCollection();
    void initPipeline();

    QPointer<Tomahawk::Database> m_database;
    QPointer<ScanManager> m_scanManager;
    QPointer<AudioEngine> m_audioEngine;
    QPointer<Servent> m_servent;
    QPointer<Tomahawk::InfoSystem::InfoSystem> m_infoSystem;
    QPointer<Tomahawk::ShortcutHandler> m_shortcutHandler;
    QPointer< Tomahawk::Accounts::AccountManager > m_accountManager;
    bool m_scrubFriendlyName;
    QString m_queuedUrl;

#ifdef LIBLASTFM_FOUND
    Scrobbler* m_scrobbler;
#endif

#ifndef TOMAHAWK_HEADLESS
    TomahawkWindow* m_mainwindow;
#endif
    QPointer<PlaydarApi> playdarApi;

    SplashWidget* m_splashWidget;

    bool m_headless;
};

Q_DECLARE_METATYPE( PairList )

#endif // TOMAHAWKAPP_H
