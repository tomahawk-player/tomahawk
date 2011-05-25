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

QVector< QString > EchonestGenerator::s_moods = QVector< QString >();
QVector< QString > EchonestGenerator::s_styles = QVector< QString >();

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
    return QStringList() << "Artist" << "Artist Description" << "Song" << "Mood" << "Style" << "Variety" << "Tempo" << "Duration" << "Loudness"
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

    // fetch style and moods
    QNetworkReply* style = Echonest::Artist::listTerms( "style" );
    connect( style, SIGNAL( finished() ), this, SLOT( stylesReceived() ) );

    QNetworkReply* moods = Echonest::Artist::listTerms( "mood" );
    connect( moods, SIGNAL( finished() ), this, SLOT( moodsReceived() ) );

    QTimer::singleShot( 20000, this, SLOT( get() ) );
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

    setProperty( "number", number ); //HACK

    connect( this, SIGNAL( paramsGenerated( Echonest::DynamicPlaylist::PlaylistParams ) ), this, SLOT( doGenerate(Echonest::DynamicPlaylist::PlaylistParams ) ) );

    try {
        getParams();
    } catch( std::runtime_error& e ) {
        qWarning() << "Got invalid controls!" << e.what();
        emit error( "Filters are not valid", e.what() );
    }
}

void
EchonestGenerator::startOnDemand()
{
    connect( this, SIGNAL( paramsGenerated( Echonest::DynamicPlaylist::PlaylistParams ) ), this, SLOT( doStartOnDemand( Echonest::DynamicPlaylist::PlaylistParams ) ) );
    try {
        getParams();
    } catch( std::runtime_error& e ) {
        qWarning() << "Got invalid controls!" << e.what();
        emit error( "Filters are not valid", e.what() );
    }
}

void
EchonestGenerator::doGenerate( const Echonest::DynamicPlaylist::PlaylistParams& paramsIn )
{
    disconnect( this, SIGNAL( paramsGenerated( Echonest::DynamicPlaylist::PlaylistParams ) ), this, SLOT( doGenerate(Echonest::DynamicPlaylist::PlaylistParams ) ) );

    int number = property( "number" ).toInt();
    setProperty( "number", QVariant() );

    Echonest::DynamicPlaylist::PlaylistParams params = paramsIn;
    params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Results, number ) );
    QNetworkReply* reply = Echonest::DynamicPlaylist::staticPlaylist( params );
    qDebug() << "Generating a static playlist from echonest!" << reply->url().toString();
    connect( reply, SIGNAL( finished() ), this, SLOT( staticFinished() ) );
}

void
EchonestGenerator::doStartOnDemand( const Echonest::DynamicPlaylist::PlaylistParams& params )
{
    disconnect( this, SIGNAL( paramsGenerated( Echonest::DynamicPlaylist::PlaylistParams ) ), this, SLOT( doStartOnDemand( Echonest::DynamicPlaylist::PlaylistParams ) ) );

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

void
EchonestGenerator::getParams() throw( std::runtime_error )
{
    Echonest::DynamicPlaylist::PlaylistParams params;
    foreach( const dyncontrol_ptr& control, m_controls ) {
        params.append( control.dynamicCast<EchonestControl>()->toENParam() );
    }

    if( appendRadioType( params ) == Echonest::DynamicPlaylist::SongRadioType ) {
        // we need to do another pass, converting all song queries to song-ids.
        m_storedParams = params;
        qDeleteAll( m_waiting );
        m_waiting.clear();

        // one query per track
        for( int i = 0; i < params.count(); i++ ) {
            const Echonest::DynamicPlaylist::PlaylistParamData param = params.value( i );

            if( param.first == Echonest::DynamicPlaylist::SongId ) { // this is a song type enum
                QString text = param.second.toString();

                Echonest::Song::SearchParams q;
                q.append( Echonest::Song::SearchParamData( Echonest::Song::Combined, text ) ); // search with the free text "combined" parameter
                QNetworkReply* r = Echonest::Song::search( q );
                r->setProperty( "index", i );
                r->setProperty( "search", text );

                m_waiting.insert( r );
                connect( r, SIGNAL( finished() ), this, SLOT( songLookupFinished() ) );
            }
        }

        if( m_waiting.isEmpty() ) {
            m_storedParams.clear();
            emit paramsGenerated( params );
        }

    } else {
        emit paramsGenerated( params );
    }
}

void
EchonestGenerator::songLookupFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );

    if( !m_waiting.contains( r ) ) // another generate/start was begun meanwhile, we're out of date
        return;

    Q_ASSERT( r );
    m_waiting.remove( r );

    QString search = r->property( "search" ).toString();
    QByteArray id;
    try {
        Echonest::SongList songs = Echonest::Song::parseSearch( r );
        if( songs.size() > 0 ) {
            id = songs.first().id();
            qDebug() << "Got ID for song:" << songs.first() << "from search:" << search;;
        } else {
            qDebug() << "Got no songs from our song id lookup.. :(. We looked for:" << search;
        }
    } catch( Echonest::ParseError& e ) {
        qWarning() << "Failed to parse song/search result:" << e.errorType() << e.what();
    }
    int idx = r->property( "index" ).toInt();
    Q_ASSERT( m_storedParams.count() >= idx );

    // replace the song text with the song id in-place
    m_storedParams[ idx ].second = id;

    if( m_waiting.isEmpty() ) { // we're done!
        emit paramsGenerated( m_storedParams );
    }
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
        if( ( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" || control->selectedType() == "Song" ) && static_cast<Echonest::DynamicPlaylist::ArtistTypeEnum>( control->match().toInt() ) != type ) {
            only = false;
        } else if( ( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" || control->selectedType() == "Song" ) && static_cast<Echonest::DynamicPlaylist::ArtistTypeEnum>( control->match().toInt() ) == type ) {
            some = true;
        }
    }
    if( some && only ) {
        return true;
    } else if( some && !only ) {
        throw std::runtime_error( "All artist and song match types must be the same" );
    }

    return false;
}

Echonest::DynamicPlaylist::ArtistTypeEnum
EchonestGenerator::appendRadioType( Echonest::DynamicPlaylist::PlaylistParams& params ) const throw( std::runtime_error )
{
    /**
     * So we try to match the best type of echonest playlist, based on the controls
     * the types are artist, artist-radio, artist-description, catalog, catalog-radio, song-radio. we don't care about the catalog ones
     *
     */

    /// 1. artist: If all the artist controls are Limit-To. If some were but not all, error out.
    /// 2. artist-description: If all the artist entries are Description. If some were but not all, error out.
    /// 3. artist-radio: If all the artist entries are Similar To. If some were but not all, error out.
    /// 4. song-radio: If all the artist entries are Similar To. If some were but not all, error out.
    if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistDescriptionType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistDescriptionType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistRadioType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistRadioType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::SongRadioType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::SongRadioType ) );
    else // no artist or song or description types. default to song-radio
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::SongRadioType ) );

    return static_cast< Echonest::DynamicPlaylist::ArtistTypeEnum >( params.last().second.toInt() );
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

    /// 1. Collect all required filters
    /// 2. Get the sorted by filter if it exists.
    QList< dyncontrol_ptr > required;
    dyncontrol_ptr sorting;
    foreach( const dyncontrol_ptr& control, allcontrols ) {
        if( control->selectedType() == "Artist" || control->selectedType() == "Artist Description" || control->selectedType() == "Song" )
            required << control;
        else if( control->selectedType() == "Sorting" )
            sorting = control;
    }
    if( !sorting.isNull() )
        allcontrols.removeAll( sorting );

    /// Skip empty artists
    QList< dyncontrol_ptr > empty;
    foreach( const dyncontrol_ptr& artist, required ) {
        QString summary = artist.dynamicCast< EchonestControl >()->summary();
        if( summary.lastIndexOf( "~" ) == summary.length() - 1 )
            empty << artist;
    }
    foreach( const dyncontrol_ptr& toremove, empty ) {
        required.removeAll( toremove );
        allcontrols.removeAll( toremove );
    }

    /// If there are no artists and no filters, show some help text
    if( required.isEmpty() && allcontrols.isEmpty() )
        sentence = "No configured filters!";

    /// Do the assembling. Start with the artists if there are any, then do all the rest.
    for( int i = 0; i < required.size(); i++ ) {
        dyncontrol_ptr artist = required.value( i );
        allcontrols.removeAll( artist ); // remove from pool while we're here

        /// Collapse artist lists
        QString center, suffix;
        QString summary = artist.dynamicCast< EchonestControl >()->summary();

        if( i == 0 ) { // if it's the first.. special casez
            center = summary.remove( "~" );
            if( required.size() == 2 ) // special case for 2, no comma. ( X and Y )
                suffix = " and ";
            else if( required.size() > 2 ) // in a list with more after
                suffix = ", ";
            else if( allcontrols.isEmpty() && sorting.isNull() ) // the last one, and no more controls, so put a period
                suffix = ".";
            else
                suffix = " ";
        } else {
            center = summary.mid( summary.indexOf( "~" ) + 1 );
            if( i == required.size() - 1 ) { // if there are more, add an " and "
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
            if( !( required.isEmpty() && allcontrols.size() == 1 ) )
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

QVector< QString >
EchonestGenerator::moods()
{
    return s_moods;
}

void
EchonestGenerator::moodsReceived()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try {
        s_moods = Echonest::Artist::parseTermList( r );
    } catch( Echonest::ParseError& e ) {
        qWarning() << "Echonest failed to parse moods list";
    }
}

QVector< QString >
EchonestGenerator::styles()
{
    return s_styles;
}

void
EchonestGenerator::stylesReceived()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try {
        s_styles = Echonest::Artist::parseTermList( r );
    } catch( Echonest::ParseError& e ) {
        qWarning() << "Echonest failed to parse styles list";
    }
}
