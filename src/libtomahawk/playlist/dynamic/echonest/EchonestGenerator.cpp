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

#include "dynamic/echonest/EchonestGenerator.h"
#include "dynamic/echonest/EchonestControl.h"
#include "dynamic/echonest/EchonestSteerer.h"
#include "query.h"
#include "utils/tomahawkutils.h"

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
    return QStringList() << "Artist" << "Artist Description" << "Variety" << "Tempo" << "Duration" << "Loudness"
                          << "Danceability" << "Energy" << "Artist Familiarity" << "Artist Hotttnesss" << "Song Hotttnesss"
                          << "Longitude" << "Latitude" <<  "Mode" << "Key" << "Sorting";
}

EchonestGenerator::EchonestGenerator ( QObject* parent )
    : GeneratorInterface ( parent )
    , m_dynPlaylist( new Echonest::DynamicPlaylist() )
    , m_steeredSinceLastTrack( false )
{
    m_type = "echonest";
    m_mode = OnDemand;
    m_logo.load( RESPATH "/images/echonest_logo.png" );
//    qDebug() << "ECHONEST:" << m_logo.size();
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
EchonestGenerator::generate( int number )
{
   // convert to an echonest query, and fire it off
   qDebug() << Q_FUNC_INFO;
   qDebug() << "Generating playlist with" << m_controls.size();
   foreach( const dyncontrol_ptr& ctrl, m_controls )
       qDebug() << ctrl->selectedType() << ctrl->match() << ctrl->input();

    try {
        Echonest::DynamicPlaylist::PlaylistParams params = getParams();

        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Results, number ) );
        QNetworkReply* reply = Echonest::DynamicPlaylist::staticPlaylist( params );
        qDebug() << "Generating a static playlist from echonest!" << reply->url().toString();
        connect( reply, SIGNAL( finished() ), this, SLOT( staticFinished() ) );
    } catch( std::runtime_error& e ) {
        qWarning() << "Got invalid controls!" << e.what();
        emit error( "Filters are not valid", e.what() );
    }
}

void
EchonestGenerator::startOnDemand()
{
    try {
        Echonest::DynamicPlaylist::PlaylistParams params = getParams();

        QNetworkReply* reply = m_dynPlaylist->start( params );
        qDebug() << "starting a dynamic playlist from echonest!" << reply->url().toString();
        connect( reply, SIGNAL( finished() ), this, SLOT( dynamicStarted() ) );
    } catch( std::runtime_error& e ) {
        qWarning() << "Got invalid controls!" << e.what();
        emit error( "Filters are not valid", e.what() );
    }
}

void
EchonestGenerator::fetchNext( int rating )
{
    if( m_dynPlaylist->sessionId().isEmpty() ) {
        // we're not currently playing, oops!
        qWarning() << Q_FUNC_INFO << "asked to fetch next dynamic song when we're not in the middle of a playlist!";
        return;
    }

    QNetworkReply* reply;
    if( m_steeredSinceLastTrack ) {
        qDebug() << "Steering dynamic playlist!" << m_steerData.first << m_steerData.second;
        reply = m_dynPlaylist->fetchNextSong( Echonest::DynamicPlaylist::DynamicControls() << m_steerData );
        m_steeredSinceLastTrack = false;
    } else {
        reply = m_dynPlaylist->fetchNextSong( rating );
    }
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
        qWarning() << "libechonest threw an error trying to parse the static playlist code" << e.errorType() << "error desc:" << e.what();

        emit error( "The Echo Nest returned an error creating the playlist", e.what() );
        return;
    }

    QList< query_ptr > queries;
    foreach( const Echonest::Song& song, songs ) {
        qDebug() << "EchonestGenerator got song:" << song;
        queries << queryFromSong( song );
    }

    emit generated( queries );
}

Echonest::DynamicPlaylist::PlaylistParams
EchonestGenerator::getParams() const throw( std::runtime_error )
{
    Echonest::DynamicPlaylist::PlaylistParams params;
    foreach( const dyncontrol_ptr& control, m_controls ) {
        params.append( control.dynamicCast<EchonestControl>()->toENParam() );
    }
    appendRadioType( params );
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
        emit error( "The Echo Nest returned an error starting the station", e.what() );
    }
}

void
EchonestGenerator::dynamicFetched()
{
    Q_ASSERT( sender() );
    Q_ASSERT( qobject_cast< QNetworkReply* >( sender() ) );
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );

    resetSteering();

    if( !m_steerer.isNull() )
        m_steerer.data()->resetSteering( true );

    try
    {
        Echonest::Song song = m_dynPlaylist->parseNextSong( reply );
        query_ptr songQuery = queryFromSong( song );
        emit nextTrackGenerated( songQuery );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "libechonest threw an error parsing the next song of the dynamic playlist:" << e.errorType() << e.what();
        emit error( "The Echo Nest returned an error getting the next song", e.what() );
    }
}

void
EchonestGenerator::steerDescription( const QString& desc )
{
    m_steeredSinceLastTrack = true;
    m_steerData.first = Echonest::DynamicPlaylist::SteerDescription;
    m_steerData.second = desc;
}

void
EchonestGenerator::steerField( const QString& field )
{
    m_steeredSinceLastTrack = true;
    m_steerData.first = Echonest::DynamicPlaylist::Steer;
    m_steerData.second = field;
}

void
EchonestGenerator::resetSteering()
{
    m_steeredSinceLastTrack = false;
    m_steerData.first = Echonest::DynamicPlaylist::Steer;
    m_steerData.second = QString();
}


bool
EchonestGenerator::onlyThisArtistType( Echonest::DynamicPlaylist::ArtistTypeEnum type ) const throw( std::runtime_error )
{
    bool only = true;
    bool some = false;

    foreach( const dyncontrol_ptr& control, m_controls ) {
        if( ( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" ) && static_cast<Echonest::DynamicPlaylist::ArtistTypeEnum>( control->match().toInt() ) != type ) {
            only = false;
        } else if( ( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" ) && static_cast<Echonest::DynamicPlaylist::ArtistTypeEnum>( control->match().toInt() ) == type ) {
            some = true;
        }
    }
    if( some && only ) {
        return true;
    } else if( some && !only ) {
        throw std::runtime_error( "All artist match types must be the same" );
    }

    return false;
}

void
EchonestGenerator::appendRadioType( Echonest::DynamicPlaylist::PlaylistParams& params ) const throw( std::runtime_error )
{
    /**
     * So we try to match the best type of echonest playlist, based on the controls
     * the types are artist, artist-radio, artist-description, catalog, catalog-radio, song-radio. we don't care about the catalog ones, and
     * we can't use the song ones since for the moment EN only accepts Song IDs, not names, and we don't want to insert an extra song.search
     * call first.
     *
     */

    /// 1. artist: If all the artist controls are Limit-To. If some were but not all, error out.
    /// 2. artist-description: If all the artist entries are Description. If some were but not all, error out.
    /// 3. artist-radio: If all the artist entries are Similar To. If some were but not all, error out.
    if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistDescriptionType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistDescriptionType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistRadioType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistRadioType ) );
}

query_ptr
EchonestGenerator::queryFromSong(const Echonest::Song& song)
{
    //         track[ "album" ] = song.release(); // TODO should we include it? can be quite specific
    return Query::get( song.artistName(), song.title(), QString(), uuid() );
}

QWidget*
EchonestGenerator::steeringWidget()
{
    if( m_steerer.isNull() ) {
        m_steerer = QWeakPointer< EchonestSteerer >( new EchonestSteerer );

        connect( m_steerer.data(), SIGNAL( steerField( QString ) ), this, SLOT( steerField( QString ) ) );
        connect( m_steerer.data(), SIGNAL( steerDescription( QString ) ), this, SLOT( steerDescription( QString ) ) );
        connect( m_steerer.data(), SIGNAL( reset() ), this, SLOT( resetSteering() ) );
    }

    return m_steerer.data();
}


QString
EchonestGenerator::sentenceSummary()
{
    /**
     * The idea is we generate an english sentence from the individual phrases of the controls. We have to follow a few rules, but othewise it's quite straightforward.
     *
     * Rules:
     *   - Sentence starts with "Songs "
     *   - Artists always go first
     *   - Separate phrases by comma, and before last phrase
     *   - sorting always at end
     *   - collapse artists. "Like X, like Y, like Z, ..." -> "Like X, Y, and Z"
     *   - skip empty artist entries
     *
     *  NOTE / TODO: In order for the sentence to be grammatically correct, we must follow the EN API rules. That means we can't have multiple of some types of filters,
     *        and all Artist types must be the same. The filters aren't checked at the moment until Generate / Play is pressed. Consider doing a check on hide as well.
     */
    QList< dyncontrol_ptr > allcontrols = m_controls;
    QString sentence = "Songs ";

    /// 1. Collect all artist filters
    /// 2. Get the sorted by filter if it exists.
    QList< dyncontrol_ptr > artists;
    dyncontrol_ptr sorting;
    foreach( const dyncontrol_ptr& control, allcontrols ) {
        if( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" )
            artists << control;
        else if( control->selectedType() == "Sorting" )
            sorting = control;
    }
    if( !sorting.isNull() )
        allcontrols.removeAll( sorting );

    /// Skip empty artists
    QList< dyncontrol_ptr > empty;
    foreach( const dyncontrol_ptr& artist, artists ) {
        QString summary = artist.dynamicCast< EchonestControl >()->summary();
        if( summary.lastIndexOf( "~" ) == summary.length() - 1 )
            empty << artist;
    }
    foreach( const dyncontrol_ptr& toremove, empty ) {
        artists.removeAll( toremove );
        allcontrols.removeAll( toremove );
    }

    /// If there are no artists and no filters, show some help text
    if( artists.isEmpty() && allcontrols.isEmpty() )
        sentence = "No configured filters!";

    /// Do the assembling. Start with the artists if there are any, then do all the rest.
    for( int i = 0; i < artists.size(); i++ ) {
        dyncontrol_ptr artist = artists.value( i );
        allcontrols.removeAll( artist ); // remove from pool while we're here

        /// Collapse artist lists
        QString center, suffix;
        QString summary = artist.dynamicCast< EchonestControl >()->summary();

        if( i == 0 ) { // if it's the first.. special casez
            center = summary.remove( "~" );
            if( artists.size() == 2 ) // special case for 2, no comma. ( X and Y )
                suffix = " and ";
            else if( artists.size() > 2 ) // in a list with more after
                suffix = ", ";
            else if( allcontrols.isEmpty() && sorting.isNull() ) // the last one, and no more controls, so put a period
                suffix = ".";
            else
                suffix = " ";
        } else {
            center = summary.mid( summary.indexOf( "~" ) + 1 );
            if( i == artists.size() - 1 ) { // if there are more, add an " and "
                if( !( allcontrols.isEmpty() && sorting.isNull() ) )
                    suffix = ", ";
                else
                    suffix = ".";
            } else
                suffix += ", and ";
        }
        sentence += center + suffix;
    }
    /// Add each filter individually
    for( int i = 0; i < allcontrols.size(); i++ ) {
        /// end case: if this is the last AND there is not a sorting filter (so this is the real last one)
        const bool last = ( i == allcontrols.size() - 1 && sorting.isNull() );
        QString prefix, suffix;
        if( last ) { // only if there is not just 1
            if( !( artists.isEmpty() && allcontrols.size() == 1 ) )
                prefix = "and ";
            suffix = ".";
        } else
            suffix = ", ";
        sentence += prefix + allcontrols.value( i ).dynamicCast< EchonestControl >()->summary() + suffix;
    }
    qDebug() << "Got artists and contents:" << sentence;

    if( !sorting.isNull() ) {
        sentence += "and " + sorting.dynamicCast< EchonestControl >()->summary() + ".";
    }
    qDebug() << "Got full summary:" << sentence;

    return sentence;
}

