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

class JSResolverHelper;
class JSResolverPrivate;
class ScriptEngine;

class DLLEXPORT JSResolver : public Tomahawk::ExternalResolverGui
{
Q_OBJECT

friend class ::JSResolverHelper;

public:
    explicit JSResolver( const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );
    virtual ~JSResolver();
    static ExternalResolver* factory( const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );

    virtual Capabilities capabilities() const;

    virtual QString name() const;
    virtual QPixmap icon() const;
    virtual unsigned int weight() const;
    virtual unsigned int timeout() const;

    virtual AccountConfigWidget* configUI() const;
    virtual void saveConfig();

    virtual ExternalResolver::ErrorState error() const;
    virtual bool running() const;
    virtual void reload();

    virtual void setIcon( const QPixmap& icon );

    virtual bool canParseUrl( const QString& url, UrlType type );

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );
    virtual void stop();
    virtual void start();

    // For ScriptCollection
    virtual void artists( const Tomahawk::collection_ptr& collection );
    virtual void albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist );
    virtual void tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album );
    // For UrlLookup
    virtual void lookupUrl( const QString& url );

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

    Q_DECLARE_PRIVATE( JSResolver )
    JSResolverPrivate* d_ptr;

};

#endif // JSRESOLVER_H
