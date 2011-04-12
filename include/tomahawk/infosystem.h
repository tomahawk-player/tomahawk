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
#include <QtCore/QVariant>
#include <QtCore/QThread>

namespace Tomahawk {

namespace InfoSystem {

class InfoSystemCache;
    
enum InfoType {
    InfoTrackID = 0,
    InfoTrackArtist,
    InfoTrackAlbum,
    InfoTrackGenre,
    InfoTrackComposer,
    InfoTrackDate,
    InfoTrackNumber,
    InfoTrackDiscNumber,
    InfoTrackBitRate,
    InfoTrackLength,
    InfoTrackSampleRate,
    InfoTrackFileSize,
    InfoTrackBPM,
    InfoTrackReplayGain,
    InfoTrackReplayPeakGain,
    InfoTrackLyrics,
    InfoTrackLocation,
    InfoTrackProfile,
    InfoTrackEnergy,
    InfoTrackDanceability,
    InfoTrackTempo,
    InfoTrackLoudness,
    
    InfoArtistID,
    InfoArtistName,
    InfoArtistBiography,
    InfoArtistBlog,
    InfoArtistFamiliarity,
    InfoArtistHotttness,
    InfoArtistImages,
    InfoArtistNews,
    InfoArtistProfile,
    InfoArtistReviews,
    InfoArtistSongs,
    InfoArtistSimilars,
    InfoArtistTerms,
    InfoArtistLinks,
    InfoArtistVideos,
    
    InfoAlbumID,
    InfoAlbumName,
    InfoAlbumArtist,
    InfoAlbumDate,
    InfoAlbumGenre,
    InfoAlbumComposer,
    InfoAlbumCoverArt,

    InfoMiscTopHotttness,
    InfoMiscTopTerms,
    
    InfoMiscSubmitNowPlaying,
    InfoMiscSubmitScrobble,
    
    InfoNoInfo
};

typedef QMap< InfoType, QVariant > InfoMap;
typedef QMap< QString, QMap< QString, QString > > InfoGenericMap;
typedef QHash< QString, QVariant > InfoCustomData;
typedef QHash< QString, QString > InfoCacheCriteria;

class InfoPlugin : public QObject
{
    Q_OBJECT
    
public:
    InfoPlugin( QObject *parent );

    virtual ~InfoPlugin()
    {
        qDebug() << Q_FUNC_INFO;
    }
    
    virtual void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomData customData ) = 0;
    
signals:
    void getCachedInfo( Tomahawk::InfoSystem::InfoCacheCriteria criteria, qint64 newMaxAge, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData );
    void updateCache( Tomahawk::InfoSystem::InfoCacheCriteria criteria, Tomahawk::InfoSystem::InfoType type, QVariant output );
    void info( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData );
    void finished( QString, Tomahawk::InfoSystem::InfoType );
    
public slots:
    //FIXME: Make pure virtual when everything supports it
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData )
    {
        Q_UNUSED( criteria );
        Q_UNUSED( caller );
        Q_UNUSED( type );
        Q_UNUSED( input );
        Q_UNUSED( customData );
    }
    
protected:
    InfoType m_type;
};

typedef QWeakPointer< InfoPlugin > InfoPluginPtr;

class InfoSystem : public QObject
{
    Q_OBJECT
     
public:
        
    InfoSystem( QObject *parent );
    ~InfoSystem();
    
    void registerInfoTypes( const InfoPluginPtr &plugin, const QSet< InfoType > &types );
    
    void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomData customData );
    void getInfo( const QString &caller, const InfoMap &input, InfoCustomData customData );
    
    InfoSystemCache* getCache() { return m_cache; }

signals:
    void info( QString caller, Tomahawk::InfoSystem::InfoType, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData );
    void finished( QString target );
    
public slots:
    void infoSlot( QString target, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData );
    
private:
    QLinkedList< InfoPluginPtr > determineOrderedMatches( const InfoType type ) const;
    
    QMap< InfoType, QLinkedList< InfoPluginPtr > > m_infoMap;
    
    // For now, statically instantiate plugins; this is just somewhere to keep them
    QLinkedList< InfoPluginPtr > m_plugins;
    
    QHash< QString, QHash< InfoType, int > > m_dataTracker;
    
    InfoSystemCache* m_cache;
    QThread* m_infoSystemCacheThreadController;
};

}

}

inline uint qHash( Tomahawk::InfoSystem::InfoCacheCriteria hash )
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
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoCustomData );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoCacheCriteria );

#endif // TOMAHAWK_INFOSYSTEM_H
