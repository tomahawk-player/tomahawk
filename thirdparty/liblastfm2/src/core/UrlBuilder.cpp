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
#include "UrlBuilder.h"
#include <QRegExp>
#include <QStringList>


QUrl
lastfm::UrlBuilder::url() const
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( host() );
    url.setEncodedPath( path );
    return url;
}


QByteArray //static
lastfm::UrlBuilder::encode( QString s )
{
    foreach (QChar c, QList<QChar>() << '%' << '&' << '/' << ';' << '+' << '#' << '"')
        if (s.contains( c ))
            // the middle step may seem odd but this is what the site does
            // eg. search for the exact string "Radiohead 2 + 2 = 5"
            return QUrl::toPercentEncoding( s ).replace( "%20", "+" ).toPercentEncoding( "", "+" );;

    return QUrl::toPercentEncoding( s.replace( ' ', '+' ), "+" );
}


QString //static
lastfm::UrlBuilder::host( const QLocale& locale )
{
    switch (locale.language())
    {
        case QLocale::Portuguese: return "www.lastfm.com.br";
        case QLocale::Turkish:    return "www.lastfm.com.tr";                    
        case QLocale::French:     return "www.lastfm.fr";
        case QLocale::Italian:    return "www.lastfm.it";
        case QLocale::German:     return "www.lastfm.de";
        case QLocale::Spanish:    return "www.lastfm.es";
        case QLocale::Polish:     return "www.lastfm.pl";
        case QLocale::Russian:    return "www.lastfm.ru";
        case QLocale::Japanese:   return "www.lastfm.jp";
        case QLocale::Swedish:    return "www.lastfm.se";
        case QLocale::Chinese:    return "cn.last.fm";
        default:                  return "www.last.fm";
    }
}


QUrl //static
lastfm::UrlBuilder::localize( QUrl url)
{
    url.setHost( url.host().replace( QRegExp("^(www.)?last.fm"), host() ) );
    return url;
}


QUrl //static
lastfm::UrlBuilder::mobilize( QUrl url )
{
    url.setHost( url.host().replace( QRegExp("^(www.)?last"), "m.last" ) );
    return url;
}
