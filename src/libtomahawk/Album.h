/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn  <uwelk@xhochy.com>
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

#pragma once
#ifndef TOMAHAWKALBUM_H
#define TOMAHAWKALBUM_H

#include <QPixmap>
#include <QFuture>

// Forward Declarations breaking QSharedPointer
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    #include "collection/Collection.h"
#endif

#include "infosystem/InfoSystem.h"
#include "DllMacro.h"
#include "Query.h"
#include "Typedefs.h"


namespace Tomahawk
{

class AlbumPrivate;
class IdThreadWorker;

class DLLEXPORT Album : public QObject
{
Q_OBJECT

public:
    static album_ptr get( const Tomahawk::artist_ptr& artist, const QString& name, bool autoCreate = false );
    static album_ptr get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );

    Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );
    Album( const QString& name, const Tomahawk::artist_ptr& artist );
    virtual ~Album();

    unsigned int id() const;
    QString name() const;
    QString sortname() const;

    artist_ptr artist() const;
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
    bool coverLoaded() const;

    QList<Tomahawk::query_ptr> tracks( ModelMode mode = Mixed, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );
    Tomahawk::playlistinterface_ptr playlistInterface( ModelMode mode, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );

    QWeakPointer< Tomahawk::Album > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::Album > weakRef );

    void loadId( bool autoCreate );

public slots:
    void deleteLater();

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    void updated();
    void coverChanged();

protected:
    QScopedPointer<AlbumPrivate> d_ptr;

private slots:
    void onTracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );

    void infoSystemInfo( const Tomahawk::InfoSystem::InfoRequestData& requestData, const QVariant& output );
    void infoSystemFinished( const QString& target );

private:
    Q_DECLARE_PRIVATE( Album )
    Q_DISABLE_COPY( Album )
    QString infoid() const;
    void setIdFuture( QFuture<unsigned int> future );

    static QHash< QString, album_wptr > s_albumsByName;
    static QHash< unsigned int, album_wptr > s_albumsById;

    friend class IdThreadWorker;
};

} // ns

Q_DECLARE_METATYPE( Tomahawk::album_ptr )

#endif
