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

#ifndef QUERY_H
#define QUERY_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QVariant>

#include "Typedefs.h"
#include "Result.h"
#include "infosystem/InfoSystem.h"

#include "DllMacro.h"

class DatabaseCommand_LogPlayback;
class DatabaseCommand_PlaybackHistory;
class DatabaseCommand_LoadPlaylistEntries;

namespace Tomahawk
{

class Resolver;

struct SocialAction
{
    QVariant action;
    QVariant value;
    QVariant timestamp;
    Tomahawk::source_ptr source;

    // Make explicit so compiler won't auto-generate, since destructor of
    // source_ptr is not defined yet (only typedef included in header)
    SocialAction();
    ~SocialAction();
    SocialAction& operator=( const SocialAction& other );
    SocialAction( const SocialAction& other );
};

struct PlaybackLog
{
    Tomahawk::source_ptr source;
    unsigned int timestamp;
    unsigned int secsPlayed;

    // Make explicit so compiler won't auto-generate, since destructor of
    // source_ptr is not defined yet (only typedef included in header)
    PlaybackLog();
    ~PlaybackLog();
    PlaybackLog& operator=( const PlaybackLog& other );
    PlaybackLog( const PlaybackLog& other );
};


class DLLEXPORT Query : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_PlaybackHistory;
friend class ::DatabaseCommand_LoadPlaylistEntries;
friend class Pipeline;

public:
    enum DescriptionMode
    { Detailed = 0, Short = 1 };

    static query_ptr get( const QString& artist, const QString& track, const QString& album, const QID& qid = QString(), bool autoResolve = true );
    static query_ptr get( const QString& query, const QID& qid );

    virtual ~Query();

    /// returns list of all results so far
    QList< result_ptr > results() const;

    /// how many results found so far?
    unsigned int numResults() const;

    QID id() const;

    /// sorter for list of results
    static bool resultSorter( const result_ptr &left, const result_ptr& right );

    /// true when a perfect result has been found (score of 1.0)
    bool solved() const { return m_solved; }
    /// true when any result has been found (score may be less than 1.0)
    bool playable() const { return m_playable; }

    QString fullTextQuery() const { return m_fullTextQuery; }
    bool isFullTextQuery() const { return !m_fullTextQuery.isEmpty(); }
    bool resolvingFinished() const { return m_resolveFinished; }
    float howSimilar( const Tomahawk::result_ptr& r );

    QPair< Tomahawk::source_ptr, unsigned int > playedBy() const;
    Tomahawk::Resolver* currentResolver() const;
    QList< QWeakPointer< Tomahawk::Resolver > > resolvedBy() const { return m_resolvers; }

    void setArtist( const QString& artist ) { m_artist = artist; updateSortNames(); }
    void setComposer( const QString& composer ) { m_composer = composer; updateSortNames(); }
    void setAlbum( const QString& album ) { m_album = album; updateSortNames(); }
    void setTrack( const QString& track ) { m_track = track; updateSortNames(); }
    void setResultHint( const QString& resultHint ) { m_resultHint = resultHint; }
    void setDuration( int duration ) { m_duration = duration; }
    void setAlbumPos( unsigned int albumpos ) { m_albumpos = albumpos; }
    void setDiscNumber( unsigned int discnumber ) { m_discnumber = discnumber; }

    bool equals( const Tomahawk::query_ptr& other ) const;

    QVariant toVariant() const;
    QString toString() const;

    QString resultHint() const { return m_resultHint; }
    QString artistSortname() const { return m_artistSortname; }
    QString albumSortname() const { return m_albumSortname; }
    QString trackSortname() const { return m_trackSortname; }
    QString artist() const { return m_artist; }
    QString composer() const { return m_composer; }
    QString album() const { return m_album; }
    QString track() const { return m_track; }

    int duration() const { return m_duration; }
    unsigned int albumpos() const { return m_albumpos; }
    unsigned int discnumber() const { return m_discnumber; }

#ifndef ENABLE_HEADLESS
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
#endif

    void setResolveFinished( bool resolved ) { m_resolveFinished = resolved; }
    void setPlayedBy( const Tomahawk::source_ptr& source, unsigned int playtime );

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

    QWeakPointer< Tomahawk::Query > weakRef() { return m_ownRef; }
    void setWeakRef( QWeakPointer< Tomahawk::Query > weakRef ) { m_ownRef = weakRef; }

signals:
    void resultsAdded( const QList<Tomahawk::result_ptr>& );
    void resultsRemoved( const Tomahawk::result_ptr& );

    void albumsAdded( const QList<Tomahawk::album_ptr>& );
    void artistsAdded( const QList<Tomahawk::artist_ptr>& );

    void resultsChanged();
    void solvedStateChanged( bool state );
    void playableStateChanged( bool state );
    void resolvingFinished( bool hasResults );

    void coverChanged();

    void socialActionsLoaded();
    void statsLoaded();
    void similarTracksLoaded();
    void updated();

public slots:
    /// (indirectly) called by resolver plugins when results are found
    void addResults( const QList< Tomahawk::result_ptr >& );
    void removeResult( const Tomahawk::result_ptr& );

    void addAlbums( const QList< Tomahawk::album_ptr >& );
    void addArtists( const QList< Tomahawk::artist_ptr >& );

    void onResolvingFinished();

    // resolve if not solved()
    void onResolverAdded();
    void onResolverRemoved();

private slots:
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

    void onResultStatusChanged();
    void refreshResults();

private:
    Query();
    explicit Query( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve );
    explicit Query( const QString& query, const QID& qid );

    void init();

    void setCurrentResolver( Tomahawk::Resolver* resolver );
    void clearResults();
    void checkResults();

    void updateSortNames();
    static int levenshtein( const QString& source, const QString& target );

    void parseSocialActions();

    QList< Tomahawk::artist_ptr > m_artists;
    QList< Tomahawk::album_ptr > m_albums;
    QList< Tomahawk::result_ptr > m_results;
    bool m_solved;
    bool m_playable;
    bool m_resolveFinished;
    mutable QID m_qid;

    QString m_artistSortname;
    QString m_composerSortName;
    QString m_albumSortname;
    QString m_trackSortname;

    QString m_artist;
    QString m_composer;
    QString m_album;
    QString m_track;
    QString m_fullTextQuery;

    int m_duration;
    unsigned int m_albumpos;
    unsigned int m_discnumber;
    QString m_resultHint;

    mutable Tomahawk::artist_ptr m_artistPtr;
    mutable Tomahawk::album_ptr m_albumPtr;

    QPair< Tomahawk::source_ptr, unsigned int > m_playedBy;
    QList< QWeakPointer< Tomahawk::Resolver > > m_resolvers;

    mutable QMutex m_mutex;
    QWeakPointer< Tomahawk::Query > m_ownRef;

    bool m_playbackHistoryLoaded;
    QList< PlaybackLog > m_playbackHistory;

    bool m_socialActionsLoaded;
    QHash< QString, QVariant > m_currentSocialActions;
    QList< SocialAction > m_allSocialActions;

    bool m_simTracksLoaded;
    QList<Tomahawk::query_ptr> m_similarTracks;
    
    mutable int m_infoJobs;
};

}; //ns

Q_DECLARE_METATYPE(Tomahawk::query_ptr);

#endif // QUERY_H
