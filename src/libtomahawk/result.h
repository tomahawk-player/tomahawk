/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef RESULT_H
#define RESULT_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>

#include "typedefs.h"

#include "dllmacro.h"

class DatabaseCommand_Resolve;
class DatabaseCommand_AllTracks;
class DatabaseCommand_AddFiles;
class DatabaseCommand_LoadFile;

namespace Tomahawk
{


struct SocialAction
{
    QVariant action;
    QVariant value;
    QVariant timestamp;
    QVariant source;
};


class DLLEXPORT Result : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_Resolve;
friend class ::DatabaseCommand_AllTracks;
friend class ::DatabaseCommand_AddFiles;
friend class ::DatabaseCommand_LoadFile;

public:
    static Tomahawk::result_ptr get( const QString& url );
    virtual ~Result();

    QVariant toVariant() const;
    QString toString() const;
    Tomahawk::query_ptr toQuery() const;

    float score() const;
    RID id() const;
    bool isOnline() const;

    collection_ptr collection() const;
    Tomahawk::artist_ptr artist() const;
    Tomahawk::album_ptr album() const;
    QString track() const { return m_track; }
    QString url() const { return m_url; }
    QString mimetype() const { return m_mimetype; }
    QString friendlySource() const;

    unsigned int duration() const { return m_duration; }
    unsigned int bitrate() const { return m_bitrate; }
    unsigned int size() const { return m_size; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int modificationTime() const { return m_modtime; }
    int year() const { return m_year; }
    bool loved() { return m_currentSocialActions[ "Love" ].toBool(); }
    QList< Tomahawk::SocialAction > allSocialActions();

    void setScore( float score ) { m_score = score; }
    void setTrackId( unsigned int id ) { m_trackId = id; }
    void setFileId( unsigned int id ) { m_fileId = id; }
    void setRID( RID id ) { m_rid = id; }
    void setCollection( const Tomahawk::collection_ptr& collection );
    void setFriendlySource( const QString& s ) { m_friendlySource = s; }
    void setArtist( const Tomahawk::artist_ptr& artist );
    void setAlbum( const Tomahawk::album_ptr& album );
    void setTrack( const QString& track ) { m_track = track; }
    void setMimetype( const QString& mimetype ) { m_mimetype = mimetype; }
    void setDuration( unsigned int duration ) { m_duration = duration; }
    void setBitrate( unsigned int bitrate ) { m_bitrate = bitrate; }
    void setSize( unsigned int size ) { m_size = size; }
    void setAlbumPos( unsigned int albumpos ) { m_albumpos = albumpos; }
    void setModificationTime( unsigned int modtime ) { m_modtime = modtime; }
    void setYear( unsigned int year ) { m_year = year; }
    void setLoved( bool loved ) { m_currentSocialActions[ "Loved" ] = loved; }
    void setAllSocialActions( QList< Tomahawk::SocialAction > socialActions );

    void loadSocialActions();
    QVariantMap attributes() const { return m_attributes; }
    void setAttributes( const QVariantMap& map ) { m_attributes = map; updateAttributes(); }

    unsigned int trackId() const { return m_trackId; }
    unsigned int fileId() const { return m_fileId; }

public slots:
    void onSocialActionsLoaded();

signals:
    // emitted when the collection this result comes from is going offline/online:
    void statusChanged();

    // emitted when social actions are loaded
    void socialActionsLoaded();

private slots:
    void onOffline();
    void onOnline();

private:
    // private constructor
    explicit Result( const QString& url );
    explicit Result();

    void updateAttributes();
    void parseSocialActions();

    mutable RID m_rid;
    collection_ptr m_collection;

    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    QString m_track;
    QString m_url;
    QString m_mimetype;
    QString m_friendlySource;

    unsigned int m_duration;
    unsigned int m_bitrate;
    unsigned int m_size;
    unsigned int m_albumpos;
    unsigned int m_modtime;
    int m_year;
    float m_score;

    QVariantMap m_attributes;

    unsigned int m_trackId, m_fileId;

    QHash< QString, QVariant > m_currentSocialActions;
    QList< SocialAction > m_allSocialActions;
};

} //ns

#endif // RESULT_H
