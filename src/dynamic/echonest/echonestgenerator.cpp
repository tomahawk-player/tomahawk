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

#include "echonest/echonestgenerator.h"
#include "echonest/echonestcontrol.h"
#include "query.h"

using namespace Tomahawk;


EchonestFactory::GeneratorFactoryInterface()
{}

GeneratorInterface* 
EchonestFactory::create()
{
    return new EchonestGenerator();
}

EchonestGenerator::EchonestGenerator ( QObject* parent ) 
    : GeneratorInterface ( parent )
    , m_type( "echonest" )
    , m_mode( OnDemand )
{
    m_typeSelectors << "Variety" << "Artist" << "Description" << "Tempo" << "Duration" << "Loudness" 
                    << "Danceability" << "Energy" << "Artist Familiarity" << "Artist Hotttnesss" << "Song Familiarity" 
                    << "Longitude" << "Latitude" <<  "Mode" << "Key" << "Sorting";
                    
}

EchonestGenerator::~EchonestGenerator()
{

}

dyncontrol_ptr 
EchonestGenerator::createControl( const QString& type ) const
{
    return dyncontrol_ptr( new EchonestControl( type, m_typeSelectors ) );
}

void 
EchonestGenerator::generate ( int number )
{
   // convert to an echonest query, and fire it off
   if( number < 0 ) { // dynamic
        
   } else { // static
       Echonest::DynamicPlaylist::PlaylistParams params;
       foreach( const dyncontrol_ptr& control, m_controls ) 
           params.append( control.dynamicCast<EchonestControl>()->toENParam() );
       
       QNetworkReply* reply = Echonest::DynamicPlaylist::staticPlaylist( params );
       connect( reply, SIGNAL( finished() ), this, SLOT( staticFinished() ) );
   }
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
        QVariantMap track;
        track[ "artist" ] = song.artistName();
//         track[ "album" ] = song.release(); // TODO should we include it? can be quite specific
        track[ "track" ] = song.title();
        queries << query_ptr( new Query( track ) );
    }
    
    emit generated( queries );
}
