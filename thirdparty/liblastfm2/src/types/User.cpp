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
#include "User.h"
#include "Track.h"
#include "../core/UrlBuilder.h"
#include "../core/XmlQuery.h"
#include <QStringList>
#include <lastfm/UserList>
#include <QAbstractNetworkCache>

using lastfm::User;
using lastfm::UserList;
using lastfm::UserDetails;
using lastfm::XmlQuery;
using lastfm::ImageSize;

User::User( const XmlQuery& xml ) 
     :AbstractType(), m_match( -1.0f )
{
    m_name = xml["name"].text();
    m_images << xml["image size=small"].text()
             << xml["image size=medium"].text()
             << xml["image size=large"].text();
    m_realName = xml["realname"].text();
}


QUrl 
User::imageUrl( ImageSize size, bool square ) const
{
    if( !square ) return m_images.value( size ); 

    QUrl url = m_images.value( size );
    QRegExp re( "/serve/(\\d*)s?/" );
    return QUrl( url.toString().replace( re, "/serve/\\1s/" ));
}


QMap<QString, QString>
User::params(const QString& method) const
{
    QMap<QString, QString> map;
    map["method"] = "user."+method;
    map["user"] = m_name;
    return map;
}


QNetworkReply*
User::getFriends( bool recentTracks, int limit, int page ) const
{
    QMap<QString, QString> map = params( "getFriends" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    if ( recentTracks ) map["recenttracks"] = "1";
    return ws::get( map );
}


QNetworkReply*
User::getFriendsListeningNow( int limit, int page ) const
{
    QMap<QString, QString> map = params( "getFriendsListeningNow" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    return ws::get( map );
}


QNetworkReply*
User::getTopTags() const
{
    return ws::get( params( "getTopTags" ) );
}


QNetworkReply*
User::getTopArtists( QString period, int limit, int page ) const
{
    QMap<QString, QString> map = params( "getTopArtists" );
    map["period"] = period;
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    return ws::get( map );
}


QNetworkReply*
User::getRecentArtists() const
{
    return ws::get( params( "getRecentArtists" ) );
}


QNetworkReply*
User::getRecentTracks( int limit , int page ) const
{
    QMap<QString, QString> map = params( "getRecentTracks" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );

    QAbstractNetworkCache* cache = lastfm::nam()->cache();
    if ( cache )
        cache->remove( lastfm::ws::url( map ) );

    return ws::get( map );
}

QNetworkReply*
User::getRecentStations( int limit, int page ) const
{
    QMap<QString, QString> map = params( "getRecentStations" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    return ws::get( map );
}


QNetworkReply*
User::getRecommendedArtists( int limit, int page ) const
{
    QMap<QString, QString> map = params( "getRecommendedArtists" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    return ws::get( map );
}


QNetworkReply*
User::getNeighbours( int limit, int page ) const
{
    QMap<QString, QString> map = params( "getNeighbours" );
    map["limit"] = QString::number( limit );
    map["page"] = QString::number( page );
    return ws::get( map );
}


QNetworkReply*
User::getPlaylists() const
{
    return ws::get( params( "getPlaylists" ) );
}


UserList //static
User::list( QNetworkReply* r )
{
    UserList users;
    try {
        XmlQuery lfm = r->readAll();
        foreach (XmlQuery e, lfm.children( "user" ))
        {
            User u( e );
            users += u;
        }

        users.total = lfm["friends"].attribute("total").toInt();
        users.page = lfm["friends"].attribute("page").toInt();
        users.perPage = lfm["friends"].attribute("perPage").toInt();
        users.totalPages = lfm["friends"].attribute("totalPages").toInt();
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
    }    
    return users;
}


QNetworkReply* //static
UserDetails::getInfo( const QString& username )
{
    QMap<QString, QString> map;
    map["method"] = "user.getInfo";
    map["user"] = username;
    return ws::post( map );
}




/*
QNetworkReply* //static
UserDetails::getRecommendedArtists()
{
    QMap<QString, QString> map;
    map["method"] = "user.getRecommendedArtists";
    return ws::post( map );
}
*/

QUrl
User::www() const
{ 
    return UrlBuilder( "user" ).slash( m_name ).url();
}

UserDetails::UserDetails()
    : User()
    , m_age( 0 )
    , m_scrobbles( 0 )
    , m_registered( QDateTime() )
    , m_isSubscriber( false )
    , m_canBootstrap( false )
{}

UserDetails::UserDetails( QNetworkReply* reply )
{
    try
    {
        XmlQuery user = XmlQuery( reply->readAll() )["user"];
        m_age = user["age"].text().toUInt();
        m_scrobbles = user["playcount"].text().toUInt();
        m_registered = QDateTime::fromTime_t(user["registered"].attribute("unixtime").toUInt());
        m_country = user["country"].text();
        m_isSubscriber = ( user["subscriber"].text() == "1" );
        m_canBootstrap = ( user["bootstrap"].text() == "1" );
        m_gender = user["gender"].text();
        m_realName = user["realname"].text();
        m_name = user["name"].text();
        m_images << user["image size=small"].text()
                 << user["image size=medium"].text()
                 << user["image size=large"].text();
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
    }
}


QString
UserDetails::getInfoString() const
{
    QString text;

    text = QObject::tr("%1").arg( m_realName.isEmpty() ? m_name : m_realName );
    if ( m_age ) text.append( QObject::tr(", %1").arg( m_age ) );
    if ( m_gender.known() ) text.append( QObject::tr(", %1").arg( m_gender.toString() ) );
    if ( !m_country.isEmpty() ) text.append( QObject::tr(", %1").arg( m_country ) );
    if ( m_scrobbles ) text.append( QObject::tr(", %L1 scrobbles").arg( m_scrobbles ) );

    return text;
}

void 
UserDetails::setScrobbleCount( quint32 scrobbleCount )
{
    m_scrobbles = scrobbleCount;
}


void
UserDetails::setDateRegistered( const QDateTime& date )
{
    m_registered = date;
}

void 
UserDetails::setImages( const QList<QUrl>& images )
{
    m_images = images;
}

void 
UserDetails::setRealName( const QString& realName )
{
    m_realName = realName;
}
void 
UserDetails::setAge( unsigned short age )
{
    m_age = age;
}

void 
UserDetails::setIsSubscriber( bool subscriber )
{
    m_isSubscriber = subscriber;
}

void 
UserDetails::setCanBootstrap( bool canBootstrap )
{
    m_canBootstrap = canBootstrap;
}

void 
UserDetails::setGender( const QString& s )
{
    m_gender = Gender( s );
}

void 
UserDetails::setCountry( const QString& country )
{
    m_country = country;
}

