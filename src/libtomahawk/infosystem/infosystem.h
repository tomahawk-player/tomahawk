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

#ifndef TOMAHAWK_INFOSYSTEM_H
#define TOMAHAWK_INFOSYSTEM_H

#include <QtCore/QCryptographicHash>
#include <QtCore/QObject>
#include <QtCore/QtDebug>
#include <QtCore/QMap>
#include <QtCore/QWeakPointer>
#include <QtCore/QSet>
#include <QtCore/QLinkedList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QThread>
#include <QtCore/QStringList>

#include "dllmacro.h"

class QNetworkAccessManager;

namespace Tomahawk {

namespace InfoSystem {

class InfoSystemCache;
class InfoSystemWorker;

enum InfoType { // as items are saved in cache, mark them here to not change them
    InfoTrackID = 0,
    InfoTrackArtist = 1,
    InfoTrackAlbum = 2,
    InfoTrackGenre = 3,
    InfoTrackComposer = 4,
    InfoTrackDate = 5,
    InfoTrackNumber = 6,
    InfoTrackDiscNumber = 7,
    InfoTrackBitRate = 8,
    InfoTrackLength = 9,
    InfoTrackSampleRate = 10,
    InfoTrackFileSize = 11,
    InfoTrackBPM = 12,
    InfoTrackReplayGain = 13,
    InfoTrackReplayPeakGain = 14,
    InfoTrackLyrics = 15,
    InfoTrackLocation = 16,
    InfoTrackProfile = 17,
    InfoTrackEnergy = 18,
    InfoTrackDanceability = 19,
    InfoTrackTempo = 20,
    InfoTrackLoudness = 21,

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
    InfoChartArtists = 51,
    InfoChartAlbums = 52, /*!< Documentation for InfoChartAlbums */
    InfoChartTracks = 53,

    InfoMiscTopHotttness = 60,
    InfoMiscTopTerms = 61,

    InfoSubmitNowPlaying = 70,
    InfoSubmitScrobble = 71,

    InfoNowPlaying = 80,
    InfoNowPaused = 81,
    InfoNowResumed = 82,
    InfoNowStopped = 83,

    InfoLove = 90,
    InfoUnLove = 91,

    InfoNotifyUser = 100,

    InfoNoInfo = 101 //WARNING: *ALWAYS* keep this last!
};

struct InfoRequestData {
    QString caller;
    Tomahawk::InfoSystem::InfoType type;
    QVariant input;
    QVariantMap customData;
};

struct ArtistTrackPair {
    QString artist;
    QString track;
};

typedef QMap< InfoType, QVariant > InfoTypeMap;
typedef QMap< InfoType, uint > InfoTimeoutMap;
typedef QMap< QString, QMap< QString, QString > > InfoGenericMap;
typedef QHash< QString, QString > InfoCriteriaHash;

class DLLEXPORT InfoPlugin : public QObject
{
    Q_OBJECT

public:
    InfoPlugin();

    virtual ~InfoPlugin();

    QSet< InfoType > supportedGetTypes() const { return m_supportedGetTypes; }
    QSet< InfoType > supportedPushTypes() const { return m_supportedPushTypes; }

signals:
    void getCachedInfo( uint requestId, Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64 newMaxAge, Tomahawk::InfoSystem::InfoRequestData requestData );
    void info( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

    void updateCache( Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output );

protected slots:
    virtual void getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData ) = 0;
    virtual void pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant data ) = 0;
    virtual void notInCacheSlot( uint requestId, Tomahawk::InfoSystem::InfoCriteriaHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData ) = 0;

    virtual void namChangedSlot( QNetworkAccessManager *nam ) = 0;

protected:
    InfoType m_type;
    QSet< InfoType > m_supportedGetTypes;
    QSet< InfoType > m_supportedPushTypes;

private:
    friend class InfoSystem;
};

typedef QWeakPointer< InfoPlugin > InfoPluginPtr;

class InfoSystemCacheThread : public QThread
{
    Q_OBJECT

public:
    InfoSystemCacheThread( QObject *parent );
    virtual ~InfoSystemCacheThread();

    void run();
    QWeakPointer< InfoSystemCache > cache() const;
    void setCache( QWeakPointer< InfoSystemCache >  cache );

private:
    QWeakPointer< InfoSystemCache > m_cache;
};

class InfoSystemWorkerThread : public QThread
{
    Q_OBJECT

public:
    InfoSystemWorkerThread( QObject *parent );
    virtual ~InfoSystemWorkerThread();

    void run();
    QWeakPointer< InfoSystemWorker > worker() const;
    void setWorker( QWeakPointer< InfoSystemWorker >  worker );

private:
    QWeakPointer< InfoSystemWorker > m_worker;
};

class DLLEXPORT InfoSystem : public QObject
{
    Q_OBJECT

public:
    static InfoSystem* instance();

    InfoSystem( QObject *parent );
    ~InfoSystem();

    void getInfo( const InfoRequestData &requestData, uint timeoutMillis = 0, bool allSources = false );
    //WARNING: if changing timeoutMillis above, also change in below function in .cpp file
    void getInfo( const QString &caller, const InfoTypeMap &inputMap, const QVariantMap &customData, const InfoTimeoutMap &timeoutMap = InfoTimeoutMap(), bool allSources = false );
    void pushInfo( const QString &caller, const InfoType type, const QVariant &input );
    void pushInfo( const QString &caller, const InfoTypeMap &input );

signals:
    void info( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void finished( QString target );

public slots:
    void newNam() const;

private:
    QWeakPointer< InfoSystemCache > m_cache;
    QWeakPointer< InfoSystemWorker > m_worker;
    InfoSystemCacheThread* m_infoSystemCacheThreadController;
    InfoSystemWorkerThread* m_infoSystemWorkerThreadController;

    static InfoSystem* s_instance;
};

}

}



inline uint qHash( Tomahawk::InfoSystem::InfoCriteriaHash hash )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QStringList keys = hash.keys();
    keys.sort();
    foreach( QString key, keys )
    {
        md5.addData( key.toUtf8() );
        md5.addData( hash[key].toUtf8() );
    }

    QString hexData = md5.result();

    uint returnval = 0;

    foreach( uint val, hexData.toUcs4() )
        returnval += val;

    return returnval;
}

Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoRequestData );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoGenericMap );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoCriteriaHash );
Q_DECLARE_METATYPE( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache > );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::ArtistTrackPair );
Q_DECLARE_METATYPE( QList<Tomahawk::InfoSystem::ArtistTrackPair> );

#endif // TOMAHAWK_INFOSYSTEM_H
