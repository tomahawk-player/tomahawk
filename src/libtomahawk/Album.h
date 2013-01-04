/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWKALBUM_H
#define TOMAHAWKALBUM_H

#include "config.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#ifndef ENABLE_HEADLESS
    #include <QtGui/QPixmap>
#endif
#include <QFuture>

#include "Typedefs.h"
#include "PlaylistInterface.h"
#include "DllMacro.h"
#include "Collection.h"
#include "infosystem/InfoSystem.h"

class IdThreadWorker;

namespace Tomahawk
{

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
    QString name() const { return m_name; }
    QString sortname() const { return m_sortname; }

    artist_ptr artist() const;
#ifndef ENABLE_HEADLESS
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
#endif
    bool coverLoaded() const { return m_coverLoaded; }

    QList<Tomahawk::query_ptr> tracks( ModelMode mode = Mixed, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );
    Tomahawk::playlistinterface_ptr playlistInterface( ModelMode mode, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );

    QWeakPointer< Tomahawk::Album > weakRef() { return m_ownRef; }
    void setWeakRef( QWeakPointer< Tomahawk::Album > weakRef ) { m_ownRef = weakRef; }

    void loadId( bool autoCreate );

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    void updated();
    void coverChanged();

private slots:
    void onTracksLoaded(Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );

    void infoSystemInfo( const Tomahawk::InfoSystem::InfoRequestData& requestData, const QVariant& output );
    void infoSystemFinished( const QString& target );

private:
    Q_DISABLE_COPY( Album )
    QString infoid() const;
    void setIdFuture( QFuture<unsigned int> future );

    mutable bool m_waitingForId;
    mutable QFuture<unsigned int> m_idFuture;
    mutable unsigned int m_id;
    QString m_name;
    QString m_sortname;

    artist_ptr m_artist;

    mutable bool m_coverLoaded;
    mutable bool m_coverLoading;
    mutable QString m_uuid;

    mutable QByteArray m_coverBuffer;
#ifndef ENABLE_HEADLESS
    mutable QPixmap* m_cover;
    mutable QHash< int, QPixmap > m_coverCache;
#endif

    QHash< Tomahawk::ModelMode, QHash< Tomahawk::collection_ptr, Tomahawk::playlistinterface_ptr > > m_playlistInterface;

    QWeakPointer< Tomahawk::Album > m_ownRef;

    static QHash< QString, album_ptr > s_albumsByName;
    static QHash< unsigned int, album_ptr > s_albumsById;

    friend class ::IdThreadWorker;
};

} // ns

Q_DECLARE_METATYPE( Tomahawk::album_ptr )

#endif
