/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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


#ifndef SCRIPTCOLLECTION_H
#define SCRIPTCOLLECTION_H

#include "ScriptPlugin.h"
#include "collection/Collection.h"
#include "collection/ArtistsRequest.h"
#include "collection/AlbumsRequest.h"

#include "Typedefs.h"
#include "DllMacro.h"

#include <QPixmap>


namespace Tomahawk
{
class ScriptAccount;

class DLLEXPORT ScriptCollection : public Collection, public ScriptPlugin
{
Q_OBJECT

    // access to ScriptObject
    friend class ScriptCommand_AllArtists;
    friend class ScriptCommand_AllAlbums;
    friend class ScriptCommand_AllTracks;
    friend class JSResolver;


public:
    explicit ScriptCollection( const scriptobject_ptr& scriptObject,
                               const source_ptr& source,
                               ScriptAccount* scriptAccount,
                               QObject* parent = nullptr );
    virtual ~ScriptCollection();

    ScriptAccount* scriptAccount() const;
    bool isOnline() const override;
    void setOnline( bool isOnline );

    /**
     * @brief setServiceName sets the name of the service that provides the ScriptCollection.
     * Please note that by default, the pretty name is the same as the resolver's name, e.g.
     * "Ampache", thus prettyName and itemName yield "Ampache Collection" and "Ampache",
     * respectively.
     * However, a resolver might want to change this string to something more appropriate and
     * different from the resolver's name, to identify the specific service rather than just the
     * resolver.
     */
    void setServiceName( const QString& name );
    QString prettyName() const override;
    QString itemName() const override;
    BackendType backendType() const override { return ScriptCollectionType; }

    void fetchIcon( const QString& iconUrl );
    void setIcon( const QPixmap& icon );
    QPixmap icon( const QSize& size ) const override;
    QPixmap bigIcon() const override;

    void setDescription( const QString& text );
    QString description() const override;

    Tomahawk::ArtistsRequest* requestArtists() override;
    Tomahawk::AlbumsRequest*  requestAlbums( const Tomahawk::artist_ptr& artist ) override;
    Tomahawk::TracksRequest*  requestTracks( const Tomahawk::album_ptr& album ) override;

    void setTrackCount( int count );
    int trackCount() const override;

    QVariantMap readMetaData();
    void parseMetaData();
    void parseMetaData( const QVariantMap& metadata );

private slots:
    void onIconFetched();

private:
    ScriptAccount* m_scriptAccount;
    QString m_servicePrettyName;
    QString m_description;
    int m_trackCount;
    QPixmap m_icon;
    bool m_isOnline;
};

} //ns

#endif // SCRIPTCOLLECTION_H
