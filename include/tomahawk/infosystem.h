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
    InfoTrackID,
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
typedef QHash< QString, QVariant > InfoCustomDataHash;
typedef QHash< QString, QString > MusixMatchHash;

class InfoPlugin : public QObject
{
    Q_OBJECT
    
public:
    InfoPlugin( QObject *parent );

    virtual ~InfoPlugin()
    {
        qDebug() << Q_FUNC_INFO;
    }
    
    virtual void getInfo( const QString &caller, const InfoType type, const QVariant &data, Tomahawk::InfoSystem::InfoCustomDataHash customData ) = 0;
    
signals:
    void info( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData );
    void getCachedInfo( QHash< QString, QString > criteria, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData );
    void finished( QString, Tomahawk::InfoSystem::InfoType );
    
//public slots:
    //void notInCacheSlot( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData ) = 0;
    
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
    
    void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomDataHash customData );
    void getInfo( const QString &caller, const InfoMap &input, InfoCustomDataHash customData );
    
    InfoSystemCache* getCache() { return m_cache; }

signals:
    void info( QString caller, Tomahawk::InfoSystem::InfoType, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData );
    void finished( QString target );
    
public slots:
    void infoSlot( QString target, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomDataHash customData );
    void finishedSlot( QString target,Tomahawk::InfoSystem::InfoType type);
    
private:
    QLinkedList< InfoPluginPtr > determineOrderedMatches( const InfoType type ) const;
    
    QMap< InfoType, QLinkedList< InfoPluginPtr > > m_infoMap;
    
    // For now, statically instantiate plugins; this is just somewhere to keep them
    QLinkedList< InfoPluginPtr > m_plugins;
    
    QHash< QString, QHash< Tomahawk::InfoSystem::InfoType, int > > m_dataTracker;
    
    InfoSystemCache* m_cache;
    QThread* m_infoSystemCacheThreadController;
};

}

}

Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoGenericMap )
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoCustomDataHash );
Q_DECLARE_METATYPE( Tomahawk::InfoSystem::MusixMatchHash )

#endif // TOMAHAWK_INFOSYSTEM_H
