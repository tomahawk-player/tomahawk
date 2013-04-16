/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QSharedPointer>
#include <QPointer>
#include <QUuid>
#include <QPair>
#include <QPersistentModelIndex>

#include <boost/function.hpp>

//template <typename T> class QSharedPointer;

#include <QNetworkReply>

namespace Tomahawk
{
    class Artist;
    class Album;
    class Collection;
    class Playlist;
    class PlaylistEntry;
    class PlaylistInterface;
    class DynamicPlaylist;
    class Query;
    class Result;
    class Source;
    class DynamicControl;
    class GeneratorInterface;
    class PeerInfo;

    typedef QSharedPointer<Collection> collection_ptr;
    typedef QSharedPointer<Playlist> playlist_ptr;
    typedef QSharedPointer<PlaylistEntry> plentry_ptr;
    typedef QSharedPointer<PlaylistInterface> playlistinterface_ptr;
    typedef QSharedPointer<DynamicPlaylist> dynplaylist_ptr;
    typedef QSharedPointer<Query> query_ptr;
    typedef QSharedPointer<Result> result_ptr;
    typedef QSharedPointer<Source> source_ptr;
    typedef QSharedPointer<Artist> artist_ptr;
    typedef QWeakPointer<Artist> artist_wptr;
    typedef QSharedPointer<Album> album_ptr;
    typedef QWeakPointer<Album> album_wptr;
    typedef QSharedPointer<PeerInfo> peerinfo_ptr;
    typedef QWeakPointer<PeerInfo> peerinfo_wptr;

    typedef QSharedPointer<DynamicControl> dyncontrol_ptr;
    typedef QSharedPointer<GeneratorInterface> geninterface_ptr;

    // let's keep these typesafe, they are different kinds of GUID:
    typedef QString QID; //query id
    typedef QString RID; //result id

    enum GeneratorMode
    {
        OnDemand = 0,
        Static
    };

    enum ModelMode
    {
        Mixed = 0,
        DatabaseMode,
        InfoSystemMode,
    };

    enum ModelTypes
    {
        TypeArtist = 0,
        TypeAlbum,
        TypeQuery,
        TypeResult
    };

    class ExternalResolver;
    typedef boost::function<Tomahawk::ExternalResolver*( QString, QStringList )> ResolverFactoryFunc;

    namespace PlaylistModes {
        enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };
        enum ViewMode { Unknown, Tree, Flat, Album };
        enum SeekRestrictions { NoSeekRestrictions, NoSeek };
        enum SkipRestrictions { NoSkipRestrictions, NoSkipForwards, NoSkipBackwards, NoSkip };
        enum RetryMode { NoRetry, Retry };
        enum LatchMode { StayOnSong, RealTime };
    }


    struct SerializedUpdater {
        QString type;
        QVariantHash customData;

        SerializedUpdater( const QString& t, const QVariantHash cd = QVariantHash() ) : type( t ), customData( cd ) {}
        SerializedUpdater() {}
    };

    typedef QMultiHash< QString, SerializedUpdater > SerializedUpdaters;
    typedef QList< SerializedUpdater > SerializedUpdaterList;

    // Yes/no questions with an associated enum value
    typedef QPair< QString, int > PlaylistDeleteQuestion;
    typedef QList< PlaylistDeleteQuestion > PlaylistDeleteQuestions;

    namespace InfoSystem
    {
        enum InfoType
        {
            // as items are saved in cache, mark them here to not change them
            InfoNoInfo = 0, //WARNING: *ALWAYS* keep this first!
            InfoTrackID = 1,
            InfoTrackArtist = 2,
            InfoTrackAlbum = 3,
            InfoTrackGenre = 4,
            InfoTrackComposer = 5,
            InfoTrackDate = 6,
            InfoTrackNumber = 7,
            InfoTrackDiscNumber = 8,
            InfoTrackBitRate = 9,
            InfoTrackLength = 10,
            InfoTrackSampleRate = 11,
            InfoTrackFileSize = 12,
            InfoTrackBPM = 13,
            InfoTrackReplayGain = 14,
            InfoTrackReplayPeakGain = 15,
            InfoTrackLyrics = 16,
            InfoTrackLocation = 17,
            InfoTrackProfile = 18,
            InfoTrackEnergy = 19,
            InfoTrackDanceability = 20,
            InfoTrackTempo = 21,
            InfoTrackLoudness = 22,
            InfoTrackSimilars = 23, // cached -- do not change

            InfoArtistID = 25,
            InfoArtistName = 26,
            InfoArtistBiography = 27,
            InfoArtistImages = 28, //cached -- do not change
            InfoArtistBlog = 29,
            InfoArtistFamiliarity = 30,
            InfoArtistHotttness = 31,
            InfoArtistSongs = 32, //cached -- do not change
            InfoArtistSimilars = 33, //cached -- do not change
            InfoArtistNews = 34,
            InfoArtistProfile = 35,
            InfoArtistReviews = 36,
            InfoArtistTerms = 37,
            InfoArtistLinks = 38,
            InfoArtistVideos = 39,
            InfoArtistReleases = 40,

            InfoAlbumID = 42,
            InfoAlbumCoverArt = 43, //cached -- do not change
            InfoAlbumName = 44,
            InfoAlbumArtist = 45,
            InfoAlbumDate = 46,
            InfoAlbumGenre = 47,
            InfoAlbumComposer = 48,
            InfoAlbumSongs = 49,

            /** \var Tomahawk::InfoSystem::InfoType Tomahawk::InfoSystem::InfoType::InfoChartCapabilities
            * Documentation for InfoChartCapabilities
            *
            * Clients of this InfoType expect a QVariant
            *
            */
            InfoChartCapabilities = 50,
            /**
            * Documentation for InfoChartArtists
            */
            InfoChart = 51,

            InfoNewReleaseCapabilities = 52,
            InfoNewRelease = 53,

            InfoMiscTopHotttness = 60,
            InfoMiscTopTerms = 61,

            InfoSubmitNowPlaying = 70,
            InfoSubmitScrobble = 71,

            InfoNowPlaying = 80,
            InfoNowPaused = 81,
            InfoNowResumed = 82,
            InfoNowStopped = 83,
            InfoTrackUnresolved = 84,

            InfoLove = 90,
            InfoUnLove = 91,
            InfoShareTrack = 92,

            InfoNotifyUser = 100,

            InfoLastInfo = 101 //WARNING: *ALWAYS* keep this last!
        };

        class InfoPlugin;

        typedef QSet< InfoType > InfoTypeSet;
        typedef QMap< InfoType, QVariant > InfoTypeMap;
        typedef QMap< InfoType, uint > InfoTimeoutMap;
        typedef QHash< QString, QString > InfoStringHash;
        typedef QPair< QVariantMap, QVariant > PushInfoPair;

        typedef QPointer< InfoPlugin > InfoPluginPtr;
    }
}; // ns

typedef int AudioErrorCode;
typedef int AudioState;
typedef QList< QPair< QString, QString > > PairList;

// creates 36char ascii guid without {} around it
inline static QString uuid()
{
    // kinda lame, but
    QString q = QUuid::createUuid().toString();
    q.remove( 0, 1 );
    q.chop( 1 );
    return q;
}

Q_DECLARE_METATYPE( QModelIndex )
Q_DECLARE_METATYPE( QPersistentModelIndex )
Q_DECLARE_METATYPE( QNetworkReply* );

#endif // TYPEDEFS_H
