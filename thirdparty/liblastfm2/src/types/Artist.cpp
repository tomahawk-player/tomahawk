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
#include "Artist.h"
#include "User.h"
#include "../core/UrlBuilder.h"
#include "../core/XmlQuery.h"
#include "../ws/ws.h"
#include <QRegExp>
#include <QStringList>

using lastfm::Artist;
using lastfm::User;
using lastfm::ImageSize;
using lastfm::XmlQuery;

QUrl
Artist::imageUrl( ImageSize size, bool square ) const
{
    if( !square ) return m_images.value( size ); 

    QUrl url = m_images.value( size );
    QRegExp re( "/serve/(\\d*)s?/" );
    return QUrl( url.toString().replace( re, "/serve/\\1s/" ));
}

static inline QList<QUrl> images( const lastfm::XmlQuery& e )
{
    QList<QUrl> images;
    images += e["image size=small"].text();
    images += e["image size=medium"].text();
    images += e["image size=large"].text();
    return images;
}


Artist::Artist( const XmlQuery& xml )
    :AbstractType()
{
    m_name = xml["name"].text();
    m_images = images( xml );
}


QMap<QString, QString> //private
Artist::params( const QString& method ) const
{
    QMap<QString, QString> map;
    map["method"] = "artist."+method;
    map["artist"] = m_name;
    return map;
}


QNetworkReply*
Artist::share( const QStringList& recipients, const QString& message, bool isPublic ) const
{
    QMap<QString, QString> map = params("share");
    map["recipient"] = recipients.join(",");
    map["public"] = isPublic ? "1" : "0";
    if (message.size()) map["message"] = message;
    return lastfm::ws::post(map);
}


QUrl 
Artist::www() const
{
    return UrlBuilder( "music" ).slash( Artist::name() ).url();
}

QNetworkReply*
Artist::getEvents(int limit) const
{
    QMap<QString, QString> map = params("getEvents");
    if (limit) map["limit"] = QString::number(limit);
    return ws::get( map );
}

QNetworkReply* 
Artist::getInfo(const QString& user, const QString& sk) const
{
    QMap<QString, QString> map = params("getInfo");
    if (!user.isEmpty()) map["username"] = user;
    if (!sk.isEmpty()) map["sk"] = sk;
    return ws::get( map );
}


QNetworkReply* 
Artist::getTags() const
{
    return ws::get( params("getTags") );
}

QNetworkReply* 
Artist::getTopTags() const
{
    return ws::get( params("getTopTags") );
}


QNetworkReply* 
Artist::getSimilar() const
{
    return ws::get( params("getSimilar") );
}


QNetworkReply* 
Artist::search( int limit ) const
{
    QMap<QString, QString> map = params("search");
    if (limit > 0) map["limit"] = QString::number(limit);
    return ws::get(map);
}


QMap<int, QString> /* static */
Artist::getSimilar( QNetworkReply* r )
{
    QMap<int, QString> artists;
    try
    {
        XmlQuery lfm = ws::parse(r);        
        foreach (XmlQuery e, lfm.children( "artist" ))
        {
            // convert floating percentage to int in range 0 to 10,000
            int const match = e["match"].text().toFloat() * 100;
            artists.insertMulti( match, e["name"].text() );
        }
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
    }
    return artists;
}



QList<Artist> /* static */
Artist::list( QNetworkReply* r )
{
    QList<Artist> artists;
    try {
        XmlQuery lfm = ws::parse(r);
        foreach (XmlQuery xq, lfm.children( "artist" )) {
            Artist artist( xq );
            artists += artist;
        }
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
    }
    return artists;
}


Artist
Artist::getInfo( QNetworkReply* r )
{
    try {
        XmlQuery lfm = ws::parse(r);
        Artist artist = lfm["artist"]["name"].text();
        artist.m_images = images( lfm["artist"] );
        return artist;
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
        return Artist();
    }
}


QNetworkReply*
Artist::addTags( const QStringList& tags ) const
{
    if (tags.isEmpty())
        return 0;
    QMap<QString, QString> map = params("addTags");
    map["tags"] = tags.join( QChar(',') );
    return ws::post(map);
}
