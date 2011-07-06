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

    InfoArtistID = 22,
    InfoArtistName = 23,
    InfoArtistBiography = 24,
    InfoArtistBlog = 25,
    InfoArtistFamiliarity = 26,
    InfoArtistHotttness = 27,
    InfoArtistImages = 28, //cached -- do not change
    InfoArtistNews = 29,
    InfoArtistProfile = 30,
    InfoArtistReviews = 31,
    InfoArtistSongs = 32, //cached -- do not change
    InfoArtistSimilars = 33, //cached -- do not change
    InfoArtistTerms = 34,
    InfoArtistLinks = 35,
    InfoArtistVideos = 36,

    InfoAlbumID = 37,
    InfoAlbumName = 38,
    InfoAlbumArtist = 39,
    InfoAlbumDate = 40,
    InfoAlbumGenre = 41,
    InfoAlbumComposer = 42,
    InfoAlbumCoverArt = 43, //cached -- do not change

    InfoMiscTopHotttness = 44,
    InfoMiscTopTerms = 45,

    InfoSubmitNowPlaying = 46,
    InfoSubmitScrobble = 47,

    InfoNowPlaying = 48,
    InfoNowPaused = 49,
    InfoNowResumed = 50,
    InfoNowStopped = 51,

    InfoNoInfo = 52,
    InfoLove = 53,
    InfoUnLove = 54,

    InfoNotifyUser = 55
};

struct InfoRequestData {
    QString caller;
    Tomahawk::InfoSystem::InfoType type;
    QVariant input;
    QVariantMap customData;
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
    void getCachedInfo( uint requestId, Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64 newMaxAge, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariantMap customData );
    void info( uint requestId, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, QVariantMap customData );

    void updateCache( Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64, Tomahawk::InfoSystem::InfoType type, QVariant output );

protected slots:
    virtual void getInfo( uint requestId, const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant data, const QVariantMap customData ) = 0;
    virtual void pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant data ) = 0;
    virtual void notInCacheSlot( uint requestId, const Tomahawk::InfoSystem::InfoCriteriaHash criteria, const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const QVariantMap customData ) = 0;

    virtual void namChangedSlot( QNetworkAccessManager *nam ) = 0;
    
protected:
    InfoType m_type;
    QSet< InfoType > m_supportedGetTypes;
    QSet< InfoType > m_supportedPushTypes;

private:
    friend class InfoSystem;
};

typedef QWeakPointer< InfoPlugin > InfoPluginPtr;

class DLLEXPORT InfoSystem : public QObject
{
    Q_OBJECT

public:
    static InfoSystem* instance();

    InfoSystem( QObject *parent );
    ~InfoSystem();

    void getInfo( const QString &caller, const InfoType type, const QVariant &input, QVariantMap customData, uint timeoutMillis = 3000 );
    void getInfo( const QString &caller, const InfoTypeMap &inputMap, QVariantMap customData, const InfoTimeoutMap &timeoutMap = InfoTimeoutMap() );
    void pushInfo( const QString &caller, const InfoType type, const QVariant &input );
    void pushInfo( const QString &caller, const InfoTypeMap &input );

signals:
    void info( QString target, Tomahawk::InfoSystem::InfoType, QVariant input, QVariant output, QVariantMap customData );
    void finished( QString target );

public slots:
    void newNam() const;

private:
    QWeakPointer< InfoSystemCache > m_cache;
    QWeakPointer< InfoSystemWorker > m_worker;
    QThread* m_infoSystemCacheThreadController;
    QThread* m_infoSystemWorkerThreadController;

    static InfoSystem* s_instance;
};

}

}

inline uint qHash( Tomahawk::InfoSystem::InfoCriteriaHash hash )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    foreach( QString key, hash.keys()  )
        md5.addData( key.toUtf8() );
    foreach( QString value, hash.values()  )
        md5.addData( value.toUtf8() );

    QString hexData = md5.result();

    uint returnval = 0;

    foreach( uint val, hexData.toUcs4() )
        returnval += val;

    return returnval;
}

Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoGenericMap );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoCriteriaHash );
Q_DECLARE_METATYPE( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache > );

#endif // TOMAHAWK_INFOSYSTEM_H
