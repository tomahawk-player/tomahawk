/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "playlist/dynamic/echonest/EchonestGenerator.h"
#include "playlist/dynamic/echonest/EchonestControl.h"
#include "playlist/dynamic/echonest/EchonestSteerer.h"
#include "Query.h"
#include "utils/TomahawkUtils.h"
#include "utils/TomahawkCache.h"
#include "TomahawkSettings.h"
#include "database/DatabaseCommand_CollectionAttributes.h"
#include "database/Database.h"
#include "utils/Logger.h"
#include "SourceList.h"
#include <QFile>
#include <QDir>
#include <QReadWriteLock>
#include <EchonestCatalogSynchronizer.h>

using namespace Tomahawk;


QStringList EchonestGenerator::s_moods = QStringList();
QStringList EchonestGenerator::s_styles = QStringList();
QStringList EchonestGenerator::s_genres = QStringList();
QNetworkReply* EchonestGenerator::s_moodsJob = 0;
QNetworkReply* EchonestGenerator::s_stylesJob = 0;
QNetworkReply* EchonestGenerator::s_genresJob = 0;

static QReadWriteLock s_moods_lock;
static QReadWriteLock s_styles_lock;
static QReadWriteLock s_genres_lock;

CatalogManager* EchonestGenerator::s_catalogs = 0;


EchonestFactory::EchonestFactory()
{
}


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
    QStringList types =  QStringList() << "Artist" << "Artist Description" << "User Radio" << "Song" << "Genre" << "Mood" << "Style" << "Adventurousness" << "Variety" << "Tempo" << "Duration" << "Loudness"
                          << "Danceability" << "Energy" << "Artist Familiarity" << "Artist Hotttnesss" << "Song Hotttnesss"
                          << "Longitude" << "Latitude" <<  "Mode" << "Key" << "Sorting" << "Song Type";

    return types;
}

CatalogManager::CatalogManager( QObject* parent )
    : QObject( parent )
{
    connect( SourceList::instance(), SIGNAL( ready() ), this, SLOT( init() ) );
}

void
CatalogManager::init()
{
    connect( EchonestCatalogSynchronizer::instance(), SIGNAL( knownCatalogsChanged() ), this, SLOT( doCatalogUpdate() ) );
    connect( SourceList::instance(), SIGNAL( ready() ), this, SLOT( doCatalogUpdate() ) );

    doCatalogUpdate();
}

void
CatalogManager::collectionAttributes( const PairList& data )
{
    QPair<QString, QString> part;
    m_catalogs.clear();

    foreach ( part, data )
    {
        if ( SourceList::instance()->get( part.first.toInt() ).isNull() )
            continue;

        const QString name = SourceList::instance()->get( part.first.toInt() )->friendlyName();
        m_catalogs.insert( name, part.second );
    }

    emit catalogsUpdated();
}

void
CatalogManager::doCatalogUpdate()
{
    QSharedPointer< DatabaseCommand > cmd( new DatabaseCommand_CollectionAttributes( DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog ) );
    connect( cmd.data(), SIGNAL( collectionAttributes( PairList ) ), this, SLOT( collectionAttributes( PairList ) ) );
    Database::instance()->enqueue( cmd );
}

QHash< QString, QString >
CatalogManager::catalogs() const
{
    return m_catalogs;
}


EchonestGenerator::EchonestGenerator ( QObject* parent )
    : GeneratorInterface ( parent )
    , m_dynPlaylist( new Echonest::DynamicPlaylist() )
{
    m_type = "echonest";
    m_mode = OnDemand;
    m_logo.load( RESPATH "/images/echonest_logo.png" );

    loadStylesMoodsAndGenres();

    connect( s_catalogs, SIGNAL( catalogsUpdated() ), this, SLOT( knownCatalogsChanged() ) );
}


EchonestGenerator::~EchonestGenerator()
{
    if ( !m_dynPlaylist->sessionId().isNull() )
    {
        // Running session, delete it
        QNetworkReply* deleteReply = m_dynPlaylist->deleteSession();
        connect( deleteReply, SIGNAL( finished() ), deleteReply, SLOT( deleteLater() ) );
    }

    delete m_dynPlaylist;
}

void
EchonestGenerator::setupCatalogs()
{
    if ( s_catalogs == 0 )
        s_catalogs = new CatalogManager( 0 );
//    qDebug() << "ECHONEST:" << m_logo.size();
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
EchonestGenerator::knownCatalogsChanged()
{
    // Refresh all contrls
    foreach( const dyncontrol_ptr& control, m_controls )
    {
        control.staticCast< EchonestControl >()->updateWidgetsFromData();
    }
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
    if ( !m_dynPlaylist->sessionId().isNull() )
    {
        // Running session, delete it
        QNetworkReply* deleteReply = m_dynPlaylist->deleteSession();
        connect( deleteReply, SIGNAL( finished() ), deleteReply, SLOT( deleteLater() ) );
    }

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

    QNetworkReply* reply = m_dynPlaylist->create( params );
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

    if ( rating > -1 )
    {
        Echonest::DynamicPlaylist::DynamicFeedback feedback;
        feedback.append( Echonest::DynamicPlaylist::DynamicFeedbackParamData( Echonest::DynamicPlaylist::RateSong, QString( "last^%1").arg( rating * 2 ).toUtf8() ) );
        QNetworkReply* reply = m_dynPlaylist->feedback( feedback );
        connect( reply, SIGNAL( finished() ), reply, SLOT( deleteLater() ) ); // we don't care about the result, just send it off
    }

    QNetworkReply* reply = m_dynPlaylist->next( 1, 0 );
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
        m_dynPlaylist->parseCreate( reply );
        fetchNext();
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

    try
    {
        Echonest::DynamicPlaylist::FetchPair fetched = m_dynPlaylist->parseNext( reply );

        if ( fetched.first.size() != 1 )
        {
            qWarning() << "Did not get any track when looking up the next song from the echo nest!";
            emit error( "No more songs from The Echo Nest available in the station", "" );
            return;
        }

        query_ptr songQuery = queryFromSong( fetched.first.first() );
        emit nextTrackGenerated( songQuery );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "libechonest threw an error parsing the next song of the dynamic playlist:" << e.errorType() << e.what();
        emit error( "The Echo Nest returned an error getting the next song", e.what() );
    }
}


QByteArray
EchonestGenerator::catalogId(const QString &collectionId)
{
    return s_catalogs->catalogs().value( collectionId ).toUtf8();
}

QStringList
EchonestGenerator::userCatalogs()
{
    return s_catalogs->catalogs().keys();
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

    /// 1. catalog-radio: If any the entries are catalog types.
    /// 2. artist: If all the artist controls are Limit-To. If some were but not all, error out.
    /// 3. artist-description: If all the artist entries are Description. If some were but not all, error out.
    /// 4. artist-radio: If all the artist entries are Similar To. If some were but not all, error out.
    /// 5. song-radio: If all the artist entries are Similar To. If some were but not all, error out.
    bool someCatalog = false;
    bool genreType = false;
    foreach( const dyncontrol_ptr& control, m_controls ) {
        if ( control->selectedType() == "User Radio" )
            someCatalog = true;
        else if ( control->selectedType() == "Genre" )
            genreType = true;
    }
    if( someCatalog )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::CatalogRadioType ) );
    else if ( genreType )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::GenreRadioType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistDescriptionType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistDescriptionType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::ArtistRadioType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistRadioType ) );
    else if( onlyThisArtistType( Echonest::DynamicPlaylist::SongRadioType ) )
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::SongRadioType ) );
    else // no artist or song or description types. default to artist-description
        params.append( Echonest::DynamicPlaylist::PlaylistParamData( Echonest::DynamicPlaylist::Type, Echonest::DynamicPlaylist::ArtistDescriptionType ) );

    return static_cast< Echonest::DynamicPlaylist::ArtistTypeEnum >( params.last().second.toInt() );
}


query_ptr
EchonestGenerator::queryFromSong( const Echonest::Song& song )
{
    //         track[ "album" ] = song.release(); // TODO should we include it? can be quite specific
    return Query::get( song.artistName(), song.title(), QString(), uuid(), false );
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
    foreach( const dyncontrol_ptr& artistOrTrack, required ) {
        QString summary = artistOrTrack.dynamicCast< EchonestControl >()->summary();
        if( summary.lastIndexOf( "~" ) == summary.length() - 1 )
            empty << artistOrTrack;
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
            } else if ( i < required.size() - 2 ) // An item in the list that is before the second to last one, don't use ", and", we only want that for the last item
                suffix += ", ";
            else
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

    if( !sorting.isNull() ) {
        sentence += "and " + sorting.dynamicCast< EchonestControl >()->summary() + ".";
    }

    return sentence;
}

void
EchonestGenerator::loadStylesMoodsAndGenres()
{
    if( !s_styles.isEmpty() && !s_moods.isEmpty() && !s_genres.isEmpty() )
        return;

    loadStyles();
    loadMoods();
    loadGenres();
}

void
EchonestGenerator::loadStyles()
{
    if ( s_styles.isEmpty() )
    {
        if ( s_styles_lock.tryLockForRead() )
        {
            QVariant styles = TomahawkUtils::Cache::instance()->getData( "EchonesGenerator", "styles" );
            s_styles_lock.unlock();
            if ( styles.isValid() && styles.canConvert< QStringList >() )
            {
                s_styles = styles.toStringList();
            }
            else
            {
                s_styles_lock.lockForWrite();
                tLog() << "Styles not in cache or too old, refetching styles ...";
                s_stylesJob = Echonest::Artist::listTerms( "style" );
                connect( s_stylesJob, SIGNAL( finished() ), this, SLOT( stylesReceived() ) );
            }
        }
        else
        {
            connect( this, SIGNAL( stylesSaved() ), this, SLOT( loadStyles() ) );
        }
    }
}

void
EchonestGenerator::loadMoods()
{
    if ( s_moods.isEmpty() )
    {
        if ( s_moods_lock.tryLockForRead() )
        {
            QVariant moods = TomahawkUtils::Cache::instance()->getData( "EchonesGenerator", "moods" );
            s_moods_lock.unlock();
            if ( moods.isValid() && moods.canConvert< QStringList >() ) {
                s_moods = moods.toStringList();
            }
            else
            {
                s_moods_lock.lockForWrite();
                tLog() << "Moods not in cache or too old, refetching moods ...";
                s_moodsJob = Echonest::Artist::listTerms( "mood" );
                connect( s_moodsJob, SIGNAL( finished() ), this, SLOT( moodsReceived() ) );
            }
        }
        else
        {
            connect( this, SIGNAL( moodsSaved() ), this, SLOT( loadMoods() ) );
        }
    }
}

void
EchonestGenerator::loadGenres()
{
    if ( s_genres.isEmpty() )
    {
        if ( s_genres_lock.tryLockForRead() )
        {
            QVariant genres = TomahawkUtils::Cache::instance()->getData( "EchonesGenerator", "genres" );
            s_genres_lock.unlock();
            if ( genres.isValid() && genres.canConvert< QStringList >() )
            {
                s_genres = genres.toStringList();
            }
            else
            {
                s_genres_lock.lockForWrite();
                tLog() << "Genres not in cache or too old, refetching genres ...";
                s_genresJob = Echonest::Artist::fetchGenres();
                connect( s_genresJob, SIGNAL( finished() ), this, SLOT( genresReceived() ) );
            }
        }
        else
        {
            connect( this, SIGNAL( genresSaved() ), this, SLOT( loadGenres() ) );
        }
    }
}

QStringList
EchonestGenerator::moods()
{
    return s_moods;
}


void
EchonestGenerator::moodsReceived()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try
    {
        s_moods = Echonest::Artist::parseTermList( r ).toList();
    }
    catch( Echonest::ParseError& e )
    {
        qWarning() << "Echonest failed to parse moods list";
    }
    s_moodsJob = 0;

    TomahawkUtils::Cache::instance()->putData( "EchonesGenerator", 1209600000 /* 2 weeks */, "moods", QVariant::fromValue< QStringList >( s_moods ) );
    s_moods_lock.unlock();
    emit moodsSaved();
}


QStringList
EchonestGenerator::styles()
{
    return s_styles;
}


void
EchonestGenerator::stylesReceived()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try
    {
        s_styles = Echonest::Artist::parseTermList( r ).toList();
    }
    catch( Echonest::ParseError& e )
    {
        qWarning() << "Echonest failed to parse styles list";
    }
    s_stylesJob = 0;

    TomahawkUtils::Cache::instance()->putData( "EchonesGenerator", 1209600000 /* 2 weeks */, "styles", QVariant::fromValue< QStringList >( s_styles ) );
    s_styles_lock.unlock();
    emit stylesSaved();
}

QStringList
EchonestGenerator::genres()
{
    return s_genres;
}

void
EchonestGenerator::genresReceived()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try
    {
        s_genres = Echonest::Artist::parseGenreList( r ).toList();
    }
    catch( Echonest::ParseError& e )
    {
        qWarning() << "Echonest failed to parse genres list";
    }
    s_genresJob = 0;

    TomahawkUtils::Cache::instance()->putData( "EchonesGenerator", 1209600000 /* 2 weeks */, "genres", QVariant::fromValue< QStringList >( s_genres ) );
    s_genres_lock.unlock();
    emit genresSaved();
}
