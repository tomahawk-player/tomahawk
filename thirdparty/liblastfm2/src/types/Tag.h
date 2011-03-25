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
#ifndef LASTFM_TAG_H
#define LASTFM_TAG_H

#include <lastfm/global.h>
#include <QMap>
#include <QString>
#include <QUrl>

namespace lastfm
{
    class LASTFM_DLLEXPORT Tag
    {
        QString m_name;
    
    public:
        Tag( const QString& name ) : m_name( name )
        {}
    
        operator QString() const { return m_name; }
        QString name() const { return m_name; }
    
        /** the global tag page at www.last.fm */
        QUrl www() const;
        /** the tag page for user @p user at www.last.fm */
        QUrl www( const class User& user ) const;
        /** pass the finished QNetworkReply to Tag::list() */
        class QNetworkReply* search() const;

        /** the top global tags on Last.fm, sorted by popularity (number of times used) */
        static class QNetworkReply* getTopTags();
    
        /** the integer is the weighting, not all list type return requests
          * have a weighting, so the int may just be zero, if you don't care
          * about the weight just do this: 
          * QStringList tags = Tag::list( reply ).values();
          */
        static QMap<int, QString> list( QNetworkReply* );
    };
}

#endif
