/*
   Copyright 2009 Last.fm Ltd. 

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

#include <QRegExp>
#include <QStringList>

#include "RadioStation.h"
#include "../core/XmlQuery.h"

QRegExp rxDisco("opt:discovery\\|(\\S+)", Qt::CaseSensitive, QRegExp::RegExp2);
QRegExp rxRep("opt:rep\\|([\\d\\.]+)", Qt::CaseSensitive, QRegExp::RegExp2);
QRegExp rxMainstr("opt:mainstr\\|([\\d\\.]+)", Qt::CaseSensitive, QRegExp::RegExp2);


const float k_defaultRep(0.5);
const float k_defaultMainstr(0.5);
const bool k_defaultDisco(false);

//static 
QList<lastfm::RadioStation> 
lastfm::RadioStation::list( QNetworkReply* r )
{
    QList<lastfm::RadioStation> result;
    try {
        foreach (XmlQuery xq, XmlQuery(ws::parse(r)).children("station")) {
            lastfm::RadioStation rs( QUrl::fromPercentEncoding( xq["url"].text().toUtf8() ) );
            rs.setTitle(xq["name"].text());
            result.append(rs);
        }
    }
    catch (ws::ParseError& e)
    {
        qWarning() << e.what();
    }
    return result;
}

void
lastfm::RadioStation::setString( const QString& string )
{
    QString replaceString( string );
    QString decodedString = QUrl::fromPercentEncoding( replaceString.replace( QChar('+'), QChar(' ') ).toUtf8() );

    QRegExp rxRql(              "lastfm:\\/\\/rql\\/(.+)$" );
    QRegExp rxPersonal(         "lastfm:\\/\\/user\\/(.+)\\/personal" );
    QRegExp rxRecommended(      "lastfm://user/(.+)\\/recommended" );
    QRegExp rxNeighbours(       "lastfm:\\/\\/user\\/(.+)\\/neighbours" );
    QRegExp rxLoved(            "lastfm:\\/\\/user\\/(.+)\\/loved" );
    QRegExp rxGlobalTags(       "lastfm:\\/\\/globaltags\\/(.+)" );
    QRegExp rxSimilarArtists(   "lastfm:\\/\\/artist\\/(.+)\\/similarartists" );
    QRegExp rxUserTags(         "lastfm:\\/\\/usertags\\/(.+)\\/(.+)" );
    QRegExp rxPlaylist(         "lastfm:\\/\\/playlist/(.+)\\/shuffle" );

    if (rxRql.indexIn(decodedString) == 0)
        setRql( QByteArray::fromBase64( rxRql.capturedTexts()[1].toAscii() ) );
    else if (rxPersonal.indexIn(decodedString) == 0)
        setRql( libraryStr( rxPersonal.capturedTexts()[1] ) );
    else if ( rxRecommended.indexIn(decodedString) == 0)
        setRql( recommendationsStr( rxRecommended.capturedTexts()[1] ) );
    else if ( rxNeighbours.indexIn(decodedString) == 0)
        setRql( neighbourhoodStr( rxNeighbours.capturedTexts()[1] ) );
    else if ( rxLoved.indexIn(decodedString) == 0)
        setRql( lovedTracksStr( rxLoved.capturedTexts()[1] ) );
    else if ( rxGlobalTags.indexIn(decodedString) == 0)
        setRql( globalTagStr( rxGlobalTags.capturedTexts()[1] ) );
    else if ( rxSimilarArtists.indexIn(decodedString) == 0)
        setRql( similarStr( rxSimilarArtists.capturedTexts()[1] ) );
    else if ( rxUserTags.indexIn(decodedString) == 0)
        setRql( userTagStr( rxUserTags.capturedTexts()[1], rxUserTags.capturedTexts()[2] ) );
    else if ( rxPlaylist.indexIn(decodedString) == 0)
        setRql( playlistStr( rxPlaylist.capturedTexts()[1].toInt() ) );
    else
    {
        m_url = string;
    }
}

bool
lastfm::RadioStation::setRep(float rep)
{
    if ( m_rql.isEmpty() )
        return false;

    int indexIn = rxRep.indexIn(m_rql);

    if ( indexIn != -1 )
    {
        if (rep != k_defaultRep)
            m_rql.replace( indexIn, rxRep.capturedTexts()[0].length(), QString("opt:rep|%1").arg(rep) );
        else
            m_rql.replace( indexIn, rxRep.capturedTexts()[0].length(), "" );
    }
    else
    {
        // the rql doesn't have rep in it
        // so append it to the end
        if (rep != k_defaultRep)
            m_rql.append( QString(" opt:rep|%1").arg(rep) );
    }

    setRql(m_rql);

    return true;
}

bool
lastfm::RadioStation::setMainstr(float mainstr)
{
    if ( m_rql.isEmpty() )
        return false;

    int indexIn = rxMainstr.indexIn(m_rql);

    if ( indexIn != -1 )
    {
        if (mainstr != k_defaultMainstr)
            m_rql.replace( indexIn, rxMainstr.capturedTexts()[0].length(), QString("opt:mainstr|%1").arg(mainstr) );
        else
            m_rql.replace( indexIn, rxMainstr.capturedTexts()[0].length(), "" );
    }
    else
    {
        // the rql doesn't have rep in it
        // so append it to the end
        if ( mainstr != k_defaultMainstr )
            m_rql.append( QString(" opt:mainstr|%1").arg(mainstr) );
    }

    setRql(m_rql);

    return true;
}

bool
lastfm::RadioStation::setDisco(bool disco)
{
    if ( m_rql.isEmpty() )
        return false;

    int indexIn = rxDisco.indexIn(m_rql);

    if ( indexIn != -1 )
    {
        if (disco)
            m_rql.replace( indexIn, rxDisco.capturedTexts()[0].length(), "opt:discovery|true" );
        else
            m_rql.replace( indexIn, rxDisco.capturedTexts()[0].length(), "" );
    }
    else
    {
        // the rql doesn't have disco in it
        // so append it to the end if it is set

        if (disco)
            m_rql.append( " opt:discovery|true" );
    }

    setRql(m_rql);

    return true;
}

float lastfm::RadioStation::rep() const
{
    if ( rxRep.indexIn(m_rql) != -1 )
        return rxRep.capturedTexts()[1].toFloat();

    return k_defaultRep;
}

float lastfm::RadioStation::mainstr() const
{
    if ( rxMainstr.indexIn(m_rql) != -1 )
        return rxMainstr.capturedTexts()[1].toFloat();

    return k_defaultMainstr;
}

bool lastfm::RadioStation::disco() const
{
    if ( rxDisco.indexIn(m_rql) != -1 )
        return rxDisco.capturedTexts()[1] == "true";

    return k_defaultDisco;
}
