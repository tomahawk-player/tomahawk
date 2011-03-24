/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Track.h"
#include "User.h"
#include "../core/UrlBuilder.h"
#include "../core/XmlQuery.h"
#include "../ws/ws.h"
#include <QFileInfo>
#include <QStringList>


lastfm::TrackData::TrackData()
             : trackNumber( 0 ),
               duration( 0 ),
               source( Track::Unknown ),
               rating( 0 ),
               fpid( -1 ),
               loved( false ),
               null( false ),
               scrobbleStatus( Track::Null ),
               scrobbleError( Track::None )
{}

lastfm::Track::Track()
    :AbstractType()
{
    d = new TrackData;
    d->null = true;
}

lastfm::Track::Track( const QDomElement& e )
    :AbstractType()
{
    d = new TrackData;

    if (e.isNull()) { d->null = true; return; }
    
    d->artist = e.namedItem( "artist" ).toElement().text();
    d->albumArtist = e.namedItem( "albumArtist" ).toElement().text();
    d->album =  e.namedItem( "album" ).toElement().text();
    d->title = e.namedItem( "track" ).toElement().text();
    d->correctedArtist = e.namedItem( "correctedArtist" ).toElement().text();
    d->correctedAlbumArtist = e.namedItem( "correctedAlbumArtist" ).toElement().text();
    d->correctedAlbum =  e.namedItem( "correctedAlbum" ).toElement().text();
    d->correctedTitle = e.namedItem( "correctedTrack" ).toElement().text();
    d->trackNumber = 0;
    d->duration = e.namedItem( "duration" ).toElement().text().toInt();
    d->url = e.namedItem( "url" ).toElement().text();
    d->rating = e.namedItem( "rating" ).toElement().text().toUInt();
    d->source = e.namedItem( "source" ).toElement().text().toInt(); //defaults to 0, or lastfm::Track::Unknown
    d->time = QDateTime::fromTime_t( e.namedItem( "timestamp" ).toElement().text().toUInt() );
    d->loved = e.namedItem( "loved" ).toElement().text().toInt();
    d->scrobbleStatus = e.namedItem( "scrobbleStatus" ).toElement().text().toInt();
    d->scrobbleError = e.namedItem( "scrobbleError" ).toElement().text().toInt();

    for (QDomElement image(e.firstChildElement("image")) ; !image.isNull() ; image = e.nextSiblingElement("image"))
    {
        d->m_images[static_cast<lastfm::ImageSize>(image.attribute("size").toInt())] = image.text();
    }

    QDomNodeList nodes = e.namedItem( "extras" ).childNodes();
    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNode n = nodes.at(i);
        QString key = n.nodeName();
        d->extras[key] = n.toElement().text();
    }
}

void
lastfm::TrackData::onLoveFinished()
{
    XmlQuery lfm = static_cast<QNetworkReply*>(sender())->readAll();
    if ( lfm.attribute( "status" ) == "ok")
        loved = true;
    emit loveToggled( loved );
}


void
lastfm::TrackData::onUnloveFinished()
{
    XmlQuery lfm = static_cast<QNetworkReply*>(sender())->readAll();
    if ( lfm.attribute( "status" ) == "ok")
        loved = false;
    emit loveToggled( loved );
}

void
lastfm::TrackData::onGotInfo()
{
    lastfm::XmlQuery lfm( static_cast<QNetworkReply*>(sender())->readAll() );

    QString imageUrl = lfm["track"]["image size=small"].text();
    if ( !imageUrl.isEmpty() ) m_images[lastfm::Small] = imageUrl;
    imageUrl = lfm["track"]["image size=medium"].text();
    if ( !imageUrl.isEmpty() ) m_images[lastfm::Medium] = imageUrl;
    imageUrl = lfm["track"]["image size=large"].text();
    if ( !imageUrl.isEmpty() ) m_images[lastfm::Large] = imageUrl;
    imageUrl = lfm["track"]["image size=extralarge"].text();
    if ( !imageUrl.isEmpty() ) m_images[lastfm::ExtraLarge] = imageUrl;
    imageUrl = lfm["track"]["image size=mega"].text();
    if ( !imageUrl.isEmpty() ) m_images[lastfm::Mega] = imageUrl;

    loved = lfm["track"]["userloved"].text().toInt();

    emit gotInfo( lfm );
    emit loveToggled( loved );

    // you should connect everytime you call getInfo
    disconnect( this, SIGNAL(gotInfo(const XmlQuery&)), 0, 0);
}


QDomElement
lastfm::Track::toDomElement( QDomDocument& xml ) const
{
    QDomElement item = xml.createElement( "track" );
    
    #define makeElement( tagname, getter ) { \
        QString v = getter; \
        if (!v.isEmpty()) \
        { \
            QDomElement e = xml.createElement( tagname ); \
            e.appendChild( xml.createTextNode( v ) ); \
            item.appendChild( e ); \
        } \
    }

    makeElement( "artist", d->artist );
    makeElement( "albumArtist", d->albumArtist );
    makeElement( "album", d->album );
    makeElement( "track", d->title );
    makeElement( "correctedArtist", d->correctedArtist );
    makeElement( "correctedAlbumArtist", d->correctedAlbumArtist );
    makeElement( "correctedAlbum", d->correctedAlbum );
    makeElement( "correctedTrack", d->correctedTitle );
    makeElement( "duration", QString::number( d->duration ) );
    makeElement( "timestamp", QString::number( d->time.toTime_t() ) );
    makeElement( "url", d->url.toString() );
    makeElement( "source", QString::number( d->source ) );
    makeElement( "rating", QString::number(d->rating) );
    makeElement( "fpId", QString::number(d->fpid) );
    makeElement( "mbId", mbid() );
    makeElement( "loved", QString::number( isLoved() ) );
    makeElement( "scrobbleStatus", QString::number( scrobbleStatus() ) );
    makeElement( "scrobbleError", QString::number( scrobbleError() ) );

    // put the images urls in the dom
    QMapIterator<lastfm::ImageSize, QUrl> imageIter( d->m_images );
    while (imageIter.hasNext()) {
        QDomElement e = xml.createElement( "image" );
        e.appendChild( xml.createTextNode( imageIter.next().value().toString() ) );
        e.setAttribute( "size", imageIter.key() );
        item.appendChild( e );
    }

    // add the extras to the dom
    QDomElement extras = xml.createElement( "extras" );
    QMapIterator<QString, QString> extrasIter( d->extras );
    while (extrasIter.hasNext()) {
        QDomElement e = xml.createElement( extrasIter.next().key() );
        e.appendChild( xml.createTextNode( extrasIter.value() ) );
        extras.appendChild( e );
    }
    item.appendChild( extras );

    return item;
}


bool
lastfm::Track::corrected() const
{
    // If any of the corrected string have been set and they are different
    // from the initial strings then this track has been corrected.
    return ( (!d->correctedTitle.isEmpty() && (d->correctedTitle != d->title))
            || (!d->correctedAlbum.isEmpty() && (d->correctedAlbum != d->album))
            || (!d->correctedArtist.isEmpty() && (d->correctedArtist != d->artist))
            || (!d->correctedAlbumArtist.isEmpty() && (d->correctedAlbumArtist != d->albumArtist)));
}

lastfm::Artist
lastfm::Track::artist( Corrections corrected ) const
{
    if ( corrected == Corrected && !d->correctedArtist.isEmpty() )
        return Artist( d->correctedArtist );

    return Artist( d->artist );
}

lastfm::Artist
lastfm::Track::albumArtist( Corrections corrected ) const
{
    if ( corrected == Corrected && !d->correctedAlbumArtist.isEmpty() )
        return Artist( d->correctedAlbumArtist );

    return Artist( d->albumArtist );
}

lastfm::Album
lastfm::Track::album( Corrections corrected ) const
{
    if ( corrected == Corrected && !d->correctedAlbum.isEmpty() )
        return Album( artist( corrected ), d->correctedAlbum );

    return Album( artist( corrected ), d->album );
}

QString
lastfm::Track::title( Corrections corrected ) const
{
    /** if no title is set, return the musicbrainz unknown identifier
      * in case some part of the GUI tries to display it anyway. Note isNull
      * returns false still. So you should have queried this! */

    if ( corrected == Corrected && !d->correctedTitle.isEmpty() )
        return d->correctedTitle;

    return d->title.isEmpty() ? "[unknown]" : d->title;
}


QUrl
lastfm::Track::imageUrl( lastfm::ImageSize size, bool square ) const
{
    if( !square ) return d->m_images.value( size );

    QUrl url = d->m_images.value( size );
    QRegExp re( "/serve/(\\d*)s?/" );
    return QUrl( url.toString().replace( re, "/serve/\\1s/" ));
}


QString
lastfm::Track::toString( const QChar& separator, Corrections corrections ) const
{
    if ( d->artist.isEmpty() )
    {
        if ( d->title.isEmpty() )
            return QFileInfo( d->url.path() ).fileName();
        else
            return title( corrections );
    }

    if ( d->title.isEmpty() )
        return artist( corrections );

    return artist( corrections ) + ' ' + separator + ' ' + title( corrections );
}


QString //static
lastfm::Track::durationString( int const duration )
{
    QTime t = QTime().addSecs( duration );
    if (duration < 60*60)
        return t.toString( "m:ss" );
    else
        return t.toString( "hh:mm:ss" );
}


QNetworkReply*
lastfm::Track::share( const QStringList& recipients, const QString& message, bool isPublic ) const
{
    QMap<QString, QString> map = params("share");
    map["recipient"] = recipients.join(",");
    map["public"] = isPublic ? "1" : "0";
    if (message.size()) map["message"] = message;
    return ws::post(map);
}


void
lastfm::MutableTrack::setFromLfm( const XmlQuery& lfm )
{
    QString imageUrl = lfm["track"]["image size=small"].text();
    if ( !imageUrl.isEmpty() ) d->m_images[lastfm::Small] = imageUrl;
    imageUrl = lfm["track"]["image size=medium"].text();
    if ( !imageUrl.isEmpty() ) d->m_images[lastfm::Medium] = imageUrl;
    imageUrl = lfm["track"]["image size=large"].text();
    if ( !imageUrl.isEmpty() ) d->m_images[lastfm::Large] = imageUrl;
    imageUrl = lfm["track"]["image size=extralarge"].text();
    if ( !imageUrl.isEmpty() ) d->m_images[lastfm::ExtraLarge] = imageUrl;
    imageUrl = lfm["track"]["image size=mega"].text();
    if ( !imageUrl.isEmpty() ) d->m_images[lastfm::Mega] = imageUrl;

    d->loved = lfm["track"]["userloved"].text().toInt();

    d->forceLoveToggled( d->loved );
}


void
lastfm::MutableTrack::love()
{
    QNetworkReply* reply = ws::post(params("love"));
    QObject::connect( reply, SIGNAL(finished()), signalProxy(), SLOT(onLoveFinished()));
}


void
lastfm::MutableTrack::unlove()
{
    QNetworkReply* reply = ws::post(params("unlove"));
    QObject::connect( reply, SIGNAL(finished()), signalProxy(), SLOT(onUnloveFinished()));
}


QNetworkReply*
lastfm::MutableTrack::ban()
{
    d->extras["rating"] = "B";
    return ws::post(params("ban"));
}


QMap<QString, QString>
lastfm::Track::params( const QString& method, bool use_mbid ) const
{
    QMap<QString, QString> map;
    map["method"] = "Track."+method;
    if (d->mbid.size() && use_mbid)
        map["mbid"] = d->mbid;
    else {
        map["artist"] = d->artist;
        map["track"] = d->title;
    }
    return map;
}


QNetworkReply*
lastfm::Track::getTopTags() const
{
    return ws::get( params("getTopTags", true) );
}


QNetworkReply*
lastfm::Track::getTopFans() const
{
    return ws::get( params("getTopFans", true) );
}


QNetworkReply*
lastfm::Track::getTags() const
{
    return ws::get( params("getTags", true) );
}

void
lastfm::Track::getInfo(const QString& user, const QString& sk) const
{
    QMap<QString, QString> map = params("getInfo", true);
    if (!user.isEmpty()) map["username"] = user;
    if (!sk.isEmpty()) map["sk"] = sk;
    QObject::connect( ws::get( map ), SIGNAL(finished()), d.data(), SLOT(onGotInfo()));
}


QNetworkReply*
lastfm::Track::addTags( const QStringList& tags ) const
{
    if (tags.isEmpty())
        return 0;
    QMap<QString, QString> map = params("addTags");
    map["tags"] = tags.join( QChar(',') );
    return ws::post(map);
}


QNetworkReply*
lastfm::Track::removeTag( const QString& tag ) const
{
    if (tag.isEmpty())
        return 0;
    QMap<QString, QString> map = params( "removeTag" );
    map["tags"] = tag;
    return ws::post(map);
}


QNetworkReply*
lastfm::Track::updateNowPlaying() const
{
    QMap<QString, QString> map = params("updateNowPlaying");
    map["duration"] = QString::number( duration() );
    if ( !album().isNull() ) map["album"] = album();
    map["context"] = extra("playerId");

    qDebug() << map;

    return ws::post(map);
}


QNetworkReply*
lastfm::Track::scrobble() const
{
    QMap<QString, QString> map = params("scrobble");
    map["duration"] = QString::number( d->duration );
    map["timestamp"] = QString::number( d->time.toTime_t() );
    map["context"] = extra("playerId");
    map["albumArtist"] = d->albumArtist;
    if ( !d->album.isEmpty() ) map["album"] = d->album;

    qDebug() << map;

    return ws::post(map);
}

QNetworkReply*
lastfm::Track::scrobble(const QList<lastfm::Track>& tracks)
{
    QMap<QString, QString> map;
    map["method"] = "track.scrobble";

    for ( int i(0) ; i < tracks.count() ; ++i )
    {
        map["duration[" + QString::number(i) + "]"] = QString::number( tracks[i].duration() );
        map["timestamp[" + QString::number(i)  + "]"] = QString::number( tracks[i].timestamp().toTime_t() );
        map["track[" + QString::number(i)  + "]"] = tracks[i].title();
        map["context[" + QString::number(i)  + "]"] = tracks[i].extra("playerId");
        if ( !tracks[i].album().isNull() ) map["album[" + QString::number(i)  + "]"] = tracks[i].album();
        map["artist[" + QString::number(i) + "]"] = tracks[i].artist();
        map["albumArtist[" + QString::number(i) + "]"] = tracks[i].albumArtist();
        if ( !tracks[i].mbid().isNull() ) map["mbid[" + QString::number(i)  + "]"] = tracks[i].mbid();
    }

    qDebug() << map;

    return ws::post(map);
}


QUrl
lastfm::Track::www() const
{
    return UrlBuilder( "music" ).slash( d->artist ).slash( album().isNull() ? QString("_") : album()).slash( d->title ).url();
}


bool
lastfm::Track::isMp3() const
{
    //FIXME really we should check the file header?
    return d->url.scheme() == "file" &&
           d->url.path().endsWith( ".mp3", Qt::CaseInsensitive );
}

void
lastfm::MutableTrack::setCorrections( QString title, QString album, QString artist, QString albumArtist )
{
    d->correctedTitle = title;
    d->correctedAlbum = album;
    d->correctedArtist = artist;
    d->correctedAlbumArtist = albumArtist;

    d->forceCorrected( toString() );
}

