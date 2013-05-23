/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef JSRESOLVER_H
#define JSRESOLVER_H

#include "ExternalResolverGui.h"
#include "Query.h"
#include "utils/TomahawkUtils.h"
#include "config.h"
#include "utils/Logger.h"

#include <QDir>
#include <QFile>
#include <QThread>
#include <QWebPage>
#include <QWebFrame>

#include "DllMacro.h"

class JSResolver;

class DLLEXPORT JSResolverHelper : public QObject
{
Q_OBJECT

public:
    JSResolverHelper( const QString& scriptPath, JSResolver* parent );
    void setResolverConfig( const QVariantMap& config );

    // Return a HMAC (md5) signature of the input text with the desired key
    Q_INVOKABLE QString hmac( const QByteArray& key, const QByteArray& input );
    Q_INVOKABLE QString md5( const QByteArray& input );

    Q_INVOKABLE void addCustomUrlHandler( const QString& protocol, const QString& callbackFuncName, const QString& isAsynchronous = "false" );
    Q_INVOKABLE void reportStreamUrl( const QString& qid, const QString& streamUrl );

    Q_INVOKABLE QByteArray base64Encode( const QByteArray& input );
    Q_INVOKABLE QByteArray base64Decode( const QByteArray& input );

    void customIODeviceFactory( const Tomahawk::result_ptr& result,
                                boost::function< void( QSharedPointer< QIODevice >& ) > callback ); // async

public slots:
    QByteArray readRaw( const QString& fileName );
    QString readBase64( const QString& fileName );
    QString readCompressed( const QString& fileName );

    QString compress( const QString& data );
    QVariantMap resolverData();

    void log( const QString& message );
    bool fakeEnv() { return false; }

    void addTrackResults( const QVariantMap& results );

    void addArtistResults( const QVariantMap& results );
    void addAlbumResults( const QVariantMap& results );
    void addAlbumTrackResults( const QVariantMap& results );

    void reportCapabilities( const QVariant& capabilities );

private:
    void returnStreamUrl( const QString& streamUrl, boost::function< void( QSharedPointer< QIODevice >& ) > callback );
    QString m_scriptPath, m_urlCallback;
    QHash< QString, boost::function< void( QSharedPointer< QIODevice >& ) > > m_streamCallbacks;
    bool m_urlCallbackIsAsync;
    QVariantMap m_resolverConfig;
    JSResolver* m_resolver;
};

class DLLEXPORT ScriptEngine : public QWebPage
{
Q_OBJECT

public:
    explicit ScriptEngine( JSResolver* parent );

    QString userAgentForUrl( const QUrl& url ) const;
    void setScriptPath( const QString& scriptPath );

public slots:
    bool shouldInterruptJavaScript();

protected:
    virtual void javaScriptConsoleMessage( const QString& message, int lineNumber, const QString& sourceID );

private slots:
    void sslErrorHandler( QNetworkReply* qnr, const QList<QSslError>& errlist );

private:
    JSResolver* m_parent;
    QString m_scriptPath;
    QString m_header;
};


class DLLEXPORT JSResolver : public Tomahawk::ExternalResolverGui
{
Q_OBJECT

friend class ::JSResolverHelper;

public:
    explicit JSResolver( const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );
    virtual ~JSResolver();
    static ExternalResolver* factory( const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );

    virtual Capabilities capabilities() const { return m_capabilities; }

    virtual QString name() const         { return m_name; }
    virtual QPixmap icon() const         { return m_icon; }
    virtual unsigned int weight() const  { return m_weight; }
    virtual unsigned int timeout() const { return m_timeout; }

    virtual AccountConfigWidget* configUI() const;
    virtual void saveConfig();

    virtual ExternalResolver::ErrorState error() const;
    virtual bool running() const;
    virtual void reload();

    virtual void setIcon( const QPixmap& icon ) { m_icon = icon; }

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );
    virtual void stop();
    virtual void start();

    // For ScriptCollection
    virtual void artists( const Tomahawk::collection_ptr& collection );
    virtual void albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist );
    virtual void tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album );

signals:
    void stopped();

private slots:
    void onCollectionIconFetched();

private:
    void init();

    void loadUi();
    void setWidgetData( const QVariant& value, QWidget* widget, const QString& property );
    QVariant widgetData( QWidget* widget, const QString& property );
    QVariantMap loadDataFromWidgets();
    void fillDataInWidgets( const QVariantMap& data );
    void onCapabilitiesChanged( Capabilities capabilities );
    void loadCollections();

    // encapsulate javascript calls
    QVariantMap resolverSettings();
    QVariantMap resolverUserConfig();
    QVariantMap resolverInit();
    QVariantMap resolverCollections();

    QList< Tomahawk::result_ptr > parseResultVariantList( const QVariantList& reslist );
    QList< Tomahawk::artist_ptr > parseArtistVariantList( const QVariantList& reslist );
    QList< Tomahawk::album_ptr >  parseAlbumVariantList(  const Tomahawk::artist_ptr& artist,
                                                          const QVariantList& reslist );

    ScriptEngine* m_engine;

    QString m_name;
    QPixmap m_icon;
    unsigned int m_weight, m_timeout;
    Capabilities m_capabilities;

    bool m_ready, m_stopped;
    ExternalResolver::ErrorState m_error;

    JSResolverHelper* m_resolverHelper;
    QPointer< AccountConfigWidget > m_configWidget;
    QList< QVariant > m_dataWidgets;
    QStringList m_requiredScriptPaths;
};

#endif // JSRESOLVER_H
