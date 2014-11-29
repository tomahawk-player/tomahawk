/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "config.h"
#include "DllMacro.h"
#include "ExternalResolverGui.h"
#include "Typedefs.h"


#include <memory> // unique_ptr
#include "ScriptEngine.h" // hack, also should be renamed to JSEngine

namespace Tomahawk
{

class JSInfoPlugin;
class JSResolverHelper;
class JSResolverPrivate;
class ScriptEngine;
class ScriptJob;
class ScriptObject;
class ScriptPlugin;

class DLLEXPORT ScriptPlugin
{
public:
    virtual ~ScriptPlugin() {}
};

class DLLEXPORT JSPlugin : public QObject, public ScriptPlugin
{
    Q_OBJECT

public:
    JSPlugin()
       : m_engine( new ScriptEngine( this ) )
    {
    }

    /**
     *  Evaluate JavaScript on the WebKit thread
     */
    Q_INVOKABLE void evaluateJavaScript( const QString& scriptSource );

    /**
     * This method must be called from the WebKit thread
     */
    QVariant evaluateJavaScriptWithResult( const QString& scriptSource );

    /**
     * Escape \ and ' in strings so they are safe to use in JavaScript
     */
    static QString escape( const QString& source );


    void loadScript( const QString& path );
    void loadScripts( const QStringList& paths );
    void addToJavaScriptWindowObject( const QString& name, QObject* object );
private:
    /**
     * Wrap the pure evaluateJavaScript call in here, while the threadings guards are in public methods
     */
    QVariant evaluateJavaScriptInternal( const QString& scriptSource );

    std::unique_ptr<ScriptEngine> m_engine;

};

class DLLEXPORT JSResolver : public Tomahawk::ExternalResolverGui
{
Q_OBJECT

friend class JSResolverHelper;

public:
    explicit JSResolver( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );
    virtual ~JSResolver();
    static ExternalResolver* factory( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );

    Capabilities capabilities() const override;

    QString name() const override;
    QPixmap icon() const override;
    unsigned int weight() const override;
    unsigned int timeout() const override;

    AccountConfigWidget* configUI() const override;
    void saveConfig() override;

    ExternalResolver::ErrorState error() const override;
    bool running() const override;
    void reload() override;

    void setIcon( const QPixmap& icon ) override;

    bool canParseUrl( const QString& url, UrlType type ) override;


public slots:
    void resolve( const Tomahawk::query_ptr& query ) override;
    void stop() override;
    void start() override;

    // For ScriptCollection
    void artists( const Tomahawk::collection_ptr& collection ) override;
    void albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist ) override;
    void tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album ) override;
    // For UrlLookup
    void lookupUrl( const QString& url ) override;

signals:
    void stopped();

protected:
    QVariant callOnResolver( const QString& scriptSource );

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

    Q_DECLARE_PRIVATE( JSResolver )
    QScopedPointer<JSResolverPrivate> d_ptr;
};

} // ns: Tomahawk
#endif // JSRESOLVER_H
