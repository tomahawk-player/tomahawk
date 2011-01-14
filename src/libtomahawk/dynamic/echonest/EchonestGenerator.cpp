/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "dynamic/echonest/EchonestGenerator.h"
#include "dynamic/echonest/EchonestControl.h"
#include "query.h"
#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;


EchonestFactory::EchonestFactory()
{}

GeneratorInterface* 
EchonestFactory::create()
{
    return new EchonestGenerator();
}

dyncontrol_ptr 
EchonestFactory::createControl( const QString& controlType )
{
    return dyncontrol_ptr( new EchonestControl( controlType, typeSelectors() ) );
}

QStringList 
EchonestFactory::typeSelectors() const
{
    return QStringList() << "Artist" << "Variety"  << "Description" << "Tempo" << "Duration" << "Loudness" 
                          << "Danceability" << "Energy" << "Artist Familiarity" << "Artist Hotttnesss" << "Song Familiarity" 
                          << "Longitude" << "Latitude" <<  "Mode" << "Key" << "Sorting";
}

EchonestGenerator::EchonestGenerator ( QObject* parent ) 
    : GeneratorInterface ( parent )
    , m_dynPlaylist( new Echonest::DynamicPlaylist() )
{
    m_type = "echonest";
    m_mode = OnDemand;
    m_logo.load( RESPATH "/images/echonest_logo.png" );
    qDebug() << "ECHONEST:" << m_logo.size();
}

EchonestGenerator::~EchonestGenerator()
{
    delete m_dynPlaylist;
}

dyncontrol_ptr 
EchonestGenerator::createControl( const QString& type )
{
    m_controls << dyncontrol_ptr( new EchonestControl( type, GeneratorFactory::typeSelectors( m_type ) ) );
    return m_controls.last();
}

QPixmap EchonestGenerator::logo()
{
    return m_logo;
}


void 
EchonestGenerator::generate ( int number )
{
   // convert to an echonest query, and fire it off
    Echonest::DynamicPlaylist::PlaylistParams params = getParams();
    params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Results, number ) );
    QNetworkReply* reply = Echonest::DynamicPlaylist::staticPlaylist( params );
    qDebug() << "Generating a static playlist from echonest!" << reply->url().toString();
    connect( reply, SIGNAL( finished() ), this, SLOT( staticFinished() ) );

}

void 
EchonestGenerator::startOnDemand()
{
    Echonest::DynamicPlaylist::PlaylistParams params = getParams();
    
    QNetworkReply* reply = m_dynPlaylist->start( params );
    qDebug() << "starting a dynamic playlist from echonest!" << reply->url().toString();
    connect( reply, SIGNAL( finished() ), this, SLOT( dynamicStarted() ) );
}

void 
EchonestGenerator::fetchNext( int rating )
{
    if( m_dynPlaylist->sessionId().isEmpty() ) {
        // we're not currently playing, oops!
        qWarning() << Q_FUNC_INFO << "asked to fetch next dynamic song when we're not in the middle of a playlist!";
        return;
    }
    
    QNetworkReply* reply = m_dynPlaylist->fetchNextSong( rating );
    qDebug() << "getting next song from echonest" << reply->url().toString();
    connect( reply, SIGNAL( finished() ), this, SLOT( dynamicFetched() ) );
}


void 
EchonestGenerator::staticFinished()
{
    Q_ASSERT( sender() );
    Q_ASSERT( qobject_cast< QNetworkReply* >( sender() ) );
    
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    
    Echonest::SongList songs;
    try {
        songs = Echonest::DynamicPlaylist::parseStaticPlaylist( reply );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "libechonest threw an error trying to parse the static playlist!" << e.errorType() << e.what();
        
        return;
    }
    
    QList< query_ptr > queries;
    foreach( const Echonest::Song& song, songs ) {
        qDebug() << "EchonestGenerator got song:" << song;
        queries << queryFromSong( song );
    }
    
    emit generated( queries );
}

Echonest::DynamicPlaylist::PlaylistParams EchonestGenerator::getParams() const
{
    Echonest::DynamicPlaylist::PlaylistParams params;
    foreach( const dyncontrol_ptr& control, m_controls ) {
        params.append( control.dynamicCast<EchonestControl>()->toENParam() );
    }
    params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, determineRadioType() ) );
    return params;
}

void 
EchonestGenerator::dynamicStarted()
{
    Q_ASSERT( sender() );
    Q_ASSERT( qobject_cast< QNetworkReply* >( sender() ) );
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    
    try 
    {
        Echonest::Song song = m_dynPlaylist->parseStart( reply );
        query_ptr songQuery = queryFromSong( song );
        emit nextTrackGenerated( songQuery );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "libechonest threw an error parsing the start of the dynamic playlist:" << e.errorType() << e.what();
        emit onDemandFailed();
    }
}

void 
EchonestGenerator::dynamicFetched()
{
    Q_ASSERT( sender() );
    Q_ASSERT( qobject_cast< QNetworkReply* >( sender() ) );
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    
    try 
    {
        Echonest::Song song = m_dynPlaylist->parseNextSong( reply );
        query_ptr songQuery = queryFromSong( song );
        emit nextTrackGenerated( songQuery );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "libechonest threw an error parsing the next song of the dynamic playlist:" << e.errorType() << e.what();
    }
}


// tries to heuristically determine what sort of radio this is based on the controls
Echonest::DynamicPlaylist::ArtistTypeEnum 
EchonestGenerator::determineRadioType() const
{
    // TODO
    return Echonest::DynamicPlaylist::ArtistRadioType;
}

query_ptr 
EchonestGenerator::queryFromSong(const Echonest::Song& song)
{
    QVariantMap track;
    track[ "artist" ] = song.artistName();
    //         track[ "album" ] = song.release(); // TODO should we include it? can be quite specific
    track[ "track" ] = song.title();
    return query_ptr( new Query( track ) );
}
