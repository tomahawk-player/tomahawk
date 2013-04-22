/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TRACK_H
#define TRACK_H

#include <QObject>
#include <QList>
#include <QVariant>

#include "Typedefs.h"
#include "infosystem/InfoSystem.h"

#include "DllMacro.h"

class DatabaseCommand_LogPlayback;
class DatabaseCommand_PlaybackHistory;

namespace Tomahawk
{

struct SocialAction
{
    QVariant action;
    QVariant value;
    QVariant timestamp;
    Tomahawk::source_ptr source;
};

struct PlaybackLog
{
    Tomahawk::source_ptr source;
    unsigned int timestamp;
    unsigned int secsPlayed;
};

class TrackPrivate;

class DLLEXPORT Track : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_PlaybackHistory;
friend class Pipeline;

public:
    enum DescriptionMode
    { Detailed = 0, Short = 1 };

    static track_ptr get( const QString& artist, const QString& track, const QString& album = QString(), int duration = 0, const QString& composer = QString(), unsigned int albumpos = 0, unsigned int discnumber = 0 );

    virtual ~Track();

    QString id() const;

    void setArtist( const QString& artist ) { m_artist = artist; updateSortNames(); }
    void setAlbum( const QString& album ) { m_album = album; updateSortNames(); }
    void setTrack( const QString& track ) { m_track = track; updateSortNames(); }
/*    void setDuration( int duration ) { m_duration = duration; }
    void setAlbumPos( unsigned int albumpos ) { m_albumpos = albumpos; }
    void setDiscNumber( unsigned int discnumber ) { m_discnumber = discnumber; }
    void setComposer( const QString& composer ) { m_composer = composer; updateSortNames(); }*/

    bool equals( const Tomahawk::track_ptr& other, bool ignoreCase = false ) const;

    QVariant toVariant() const;
    QString toString() const;
    Tomahawk::query_ptr toQuery();

    QString artistSortname() const { return m_artistSortname; }
    QString composerSortname() const { return m_composerSortname; }
    QString albumSortname() const { return m_albumSortname; }
    QString trackSortname() const { return m_trackSortname; }

    QString artist() const { return m_artist; }
    QString composer() const { return m_composer; }
    QString album() const { return m_album; }
    QString track() const { return m_track; }
    int duration() const { return m_duration; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int discnumber() const { return m_discnumber; }

    Tomahawk::artist_ptr artistPtr() const;
    Tomahawk::album_ptr albumPtr() const;
    Tomahawk::artist_ptr composerPtr() const;

#ifndef ENABLE_HEADLESS
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
#endif
    bool coverLoaded() const;

    void setLoved( bool loved );
    bool loved();

    void loadStats();
    QList< Tomahawk::PlaybackLog > playbackHistory( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() ) const;
    void setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData );
    unsigned int playbackCount( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() );

    void loadSocialActions();
    QList< Tomahawk::SocialAction > allSocialActions() const;
    void setAllSocialActions( const QList< Tomahawk::SocialAction >& socialActions );
    QString socialActionDescription( const QString& action, DescriptionMode mode ) const;

    QList<Tomahawk::query_ptr> similarTracks() const;
    QStringList lyrics() const;

    QWeakPointer< Tomahawk::Track > weakRef() { return m_ownRef; }
    void setWeakRef( QWeakPointer< Tomahawk::Track > weakRef ) { m_ownRef = weakRef; }

signals:
    void coverChanged();
    void socialActionsLoaded();
    void statsLoaded();
    void similarTracksLoaded();
    void lyricsLoaded();

    void updated();

public slots:
    void deleteLater();

private slots:
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

private:
    explicit Track( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber );

    void updateSortNames();
    void parseSocialActions();

    QString m_artistSortname;
    QString m_composerSortname;
    QString m_albumSortname;
    QString m_trackSortname;

    QString m_artist;
    QString m_composer;
    QString m_album;
    QString m_track;

    int m_duration;
    unsigned int m_albumpos;
    unsigned int m_discnumber;

    mutable Tomahawk::artist_ptr m_artistPtr;
    mutable Tomahawk::album_ptr m_albumPtr;
    mutable Tomahawk::artist_ptr m_composerPtr;

    bool m_playbackHistoryLoaded;
    QList< PlaybackLog > m_playbackHistory;

    bool m_socialActionsLoaded;
    QHash< QString, QVariant > m_currentSocialActions;
    QList< SocialAction > m_allSocialActions;

    bool m_simTracksLoaded;
    QList<Tomahawk::query_ptr> m_similarTracks;

    bool m_lyricsLoaded;
    QStringList m_lyrics;

    mutable int m_infoJobs;
    mutable QString m_uuid;

    query_wptr m_query;
    QWeakPointer< Tomahawk::Track > m_ownRef;
};

}; //ns

Q_DECLARE_METATYPE( QList<Tomahawk::PlaybackLog> );
Q_DECLARE_METATYPE( Tomahawk::track_ptr );

#endif // TRACK_H
