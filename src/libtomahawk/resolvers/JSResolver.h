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
    explicit JSResolver( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );
    virtual ~JSResolver();
    static ExternalResolver* factory( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths = QStringList() );

    Capabilities capabilities() const Q_DECL_OVERRIDE;

    QString name() const Q_DECL_OVERRIDE;
    QPixmap icon() const Q_DECL_OVERRIDE;
    unsigned int weight() const Q_DECL_OVERRIDE;
    unsigned int timeout() const Q_DECL_OVERRIDE;

    AccountConfigWidget* configUI() const Q_DECL_OVERRIDE;
    void saveConfig() Q_DECL_OVERRIDE;

    ExternalResolver::ErrorState error() const Q_DECL_OVERRIDE;
    bool running() const Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;

    void setIcon( const QPixmap& icon ) Q_DECL_OVERRIDE;

    bool canParseUrl( const QString& url, UrlType type ) Q_DECL_OVERRIDE;

public slots:
    void resolve( const Tomahawk::query_ptr& query ) Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    void start() Q_DECL_OVERRIDE;

    // For ScriptCollection
    void artists( const Tomahawk::collection_ptr& collection ) Q_DECL_OVERRIDE;
    void albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist ) Q_DECL_OVERRIDE;
    void tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album ) Q_DECL_OVERRIDE;
    // For UrlLookup
    void lookupUrl( const QString& url ) Q_DECL_OVERRIDE;

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
    QScopedPointer<JSResolverPrivate> d_ptr;
};

#endif // JSRESOLVER_H
