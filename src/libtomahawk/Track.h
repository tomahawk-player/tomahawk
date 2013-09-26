/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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
#ifndef TRACK_H
#define TRACK_H

#include "DllMacro.h"
#include "PlaybackLog.h"
#include "SocialAction.h"
#include "Typedefs.h"

#include <QList>
#include <QVariant>


namespace Tomahawk
{

class DatabaseCommand_LoadInboxEntries;
class TrackPrivate;

class DLLEXPORT Track : public QObject
{
Q_OBJECT

friend class Pipeline;
friend class DatabaseCommand_LoadInboxEntries; // for setAllSocialActions

public:
    enum DescriptionMode
    { Detailed = 0, Short = 1 };

    static track_ptr get( const QString& artist, const QString& track, const QString& album = QString(), int duration = 0, const QString& composer = QString(), unsigned int albumpos = 0, unsigned int discnumber = 0 );
    static track_ptr get( unsigned int id, const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber );

    virtual ~Track();

    void setArtist( const QString& artist );
    void setAlbum( const QString& album );
    void setTrack( const QString& track );

    void setAlbumPos( unsigned int albumpos );
    // void setDuration( int duration ) { m_duration = duration; }
    // void setDiscNumber( unsigned int discnumber ) { m_discnumber = discnumber; }
    // void setComposer( const QString& composer ) { m_composer = composer; updateSortNames(); }

    bool equals( const Tomahawk::track_ptr& other, bool ignoreCase = false ) const;

    QVariant toVariant() const;
    QString toString() const;
    Tomahawk::query_ptr toQuery();

    QString composerSortname() const;
    QString albumSortname() const;
    QString artistSortname() const;
    QString trackSortname() const;

    QString artist() const;
    QString track() const;
    QString composer() const;
    QString album() const;
    int duration() const;
    int year() const;
    unsigned int albumpos() const;
    unsigned int discnumber() const;

    Tomahawk::artist_ptr artistPtr() const;
    Tomahawk::album_ptr albumPtr() const;
    Tomahawk::artist_ptr composerPtr() const;

#ifndef ENABLE_HEADLESS
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
#endif
    bool coverLoaded() const;

    void setLoved( bool loved, bool postToInfoSystem = true );
    bool loved();

    void share( const Tomahawk::source_ptr& source );

    void loadAttributes();
    QVariantMap attributes() const;
    void setAttributes( const QVariantMap& map );

    void loadStats();
    QList< Tomahawk::PlaybackLog > playbackHistory( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() ) const;
    unsigned int playbackCount( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() );

    unsigned int chartPosition() const;
    unsigned int chartCount() const;

    void loadSocialActions( bool force = false );
    QList< Tomahawk::SocialAction > allSocialActions() const;
    QList< Tomahawk::SocialAction > socialActions( const QString& actionName, const QVariant& value = QVariant(), bool filterDupeSourceNames = false );
    QString socialActionDescription( const QString& actionName, DescriptionMode mode ) const;

    QList<Tomahawk::query_ptr> similarTracks() const;
    QStringList lyrics() const;

    unsigned int trackId() const;

    QWeakPointer< Tomahawk::Track > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::Track > weakRef );

    void startPlaying();
    void finishPlaying( int timeElapsed );

    void markAsListened();
    bool isListened() const;

signals:
    void coverChanged();
    void socialActionsLoaded();
    void attributesLoaded();
    void statsLoaded();
    void similarTracksLoaded();
    void lyricsLoaded();

    void updated();

public slots:
    void deleteLater();

protected:
    QScopedPointer<TrackPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE( Track )
    explicit Track( unsigned int id, const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber );
    explicit Track( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber );

    void init();

    void updateSortNames();

    void setAllSocialActions( const QList< SocialAction >& socialActions );

    static QHash< QString, track_wptr > s_tracksByName;
};

} // namespace Tomahawk

Q_DECLARE_METATYPE( Tomahawk::track_ptr )

#endif // TRACK_H
