/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef QTSCRIPTRESOLVER_H
#define QTSCRIPTRESOLVER_H

#include "ExternalResolverGui.h"
#include "Query.h"
#include "utils/TomahawkUtils.h"
#include "config.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#ifdef QCA2_FOUND
#include <QtCrypto>
#endif

#include "DllMacro.h"

class QtScriptResolver;

class DLLEXPORT QtScriptResolverHelper : public QObject
{
Q_OBJECT

public:
    QtScriptResolverHelper( const QString& scriptPath, QtScriptResolver* parent );
    void setResolverConfig( const QVariantMap& config );

    // Return a HMAC (md5) signature of the input text with the desired key
    Q_INVOKABLE QString hmac( const QByteArray& key, const QByteArray& input );
    Q_INVOKABLE QString md5( const QByteArray& input );

    Q_INVOKABLE void addCustomUrlHandler( const QString& protocol, const QString& callbackFuncName );

    Q_INVOKABLE QByteArray base64Encode( const QByteArray& input );
    Q_INVOKABLE QByteArray base64Decode( const QByteArray& input );

    QSharedPointer<QIODevice> customIODeviceFactory( const Tomahawk::result_ptr& result );

public slots:
    QByteArray readRaw( const QString& fileName );
    QString readBase64( const QString& fileName );
    QString readCompressed( const QString& fileName );

    QString compress( const QString& data );
    QVariantMap resolverData();

    void log( const QString& message );
    bool fakeEnv() { return false; }

    void addTrackResults( const QVariantMap& results );

private:
    QString m_scriptPath, m_urlCallback;
    QVariantMap m_resolverConfig;
    QtScriptResolver* m_resolver;
#ifdef QCA2_FOUND
    QCA::Initializer m_qcaInit;
#endif
};

class DLLEXPORT ScriptEngine : public QWebPage
{
Q_OBJECT

public:
    explicit ScriptEngine( QtScriptResolver* parent )
        : QWebPage( (QObject*) parent )
        , m_parent( parent )
    {
        settings()->setAttribute( QWebSettings::OfflineStorageDatabaseEnabled, true );
        settings()->setOfflineStoragePath( TomahawkUtils::appDataDir().path() );
        settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
        settings()->setLocalStoragePath( TomahawkUtils::appDataDir().path() );
        settings()->setAttribute( QWebSettings::LocalStorageDatabaseEnabled, true );
        settings()->setAttribute( QWebSettings::LocalContentCanAccessFileUrls, true );
        settings()->setAttribute( QWebSettings::LocalContentCanAccessRemoteUrls, true );
    }

    void setScriptPath( const QString& scriptPath )
    {
        m_scriptPath = scriptPath;
    }

public slots:
    bool shouldInterruptJavaScript()
    {
        return true;
    }

protected:
    virtual void javaScriptConsoleMessage( const QString& message, int lineNumber, const QString& sourceID );

private:
    QtScriptResolver* m_parent;
    QString m_scriptPath;
};


class DLLEXPORT QtScriptResolver : public Tomahawk::ExternalResolverGui
{
Q_OBJECT

friend class ::QtScriptResolverHelper;

public:
    explicit QtScriptResolver( const QString& scriptPath );
    virtual ~QtScriptResolver();
    static ExternalResolver* factory( const QString& scriptPath );

    virtual QString name() const         { return m_name; }
    virtual QPixmap icon() const         { return m_icon; }
    virtual unsigned int weight() const  { return m_weight; }
    virtual unsigned int timeout() const { return m_timeout; }

    virtual QWidget* configUI() const;
    virtual void saveConfig();

    virtual ExternalResolver::ErrorState error() const;
    virtual bool running() const;
    virtual void reload();

    virtual void setIcon( const QPixmap& icon ) { m_icon = icon; }
public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );
    virtual void stop();
    virtual void start();

signals:
    void stopped();

private:
    void init();

    void loadUi();
    void setWidgetData( const QVariant& value, QWidget* widget, const QString& property );
    QVariant widgetData( QWidget* widget, const QString& property );
    QVariantMap loadDataFromWidgets();
    void fillDataInWidgets( const QVariantMap& data );

    // encapsulate javascript calls
    QVariantMap resolverSettings();
    QVariantMap resolverUserConfig();
    QVariantMap resolverInit();

    QList< Tomahawk::result_ptr > parseResultVariantList( const QVariantList& reslist );

    ScriptEngine* m_engine;

    QString m_name;
    QPixmap m_icon;
    unsigned int m_weight, m_timeout;

    bool m_ready, m_stopped;
    ExternalResolver::ErrorState m_error;

    QtScriptResolverHelper* m_resolverHelper;
    QWeakPointer< QWidget > m_configWidget;
    QList< QVariant > m_dataWidgets;
};

#endif // QTSCRIPTRESOLVER_H
