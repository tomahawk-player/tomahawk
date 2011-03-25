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
#ifndef LASTFM_ALBUM_H
#define LASTFM_ALBUM_H

#include <lastfm/Artist>
#include <lastfm/Mbid>
#include <QString>
#include <QUrl>

namespace lastfm
{
    class LASTFM_DLLEXPORT Album
    {
        Mbid m_mbid;
        Artist m_artist;
        QString m_title;

    public:
        Album()
        {}

        explicit Album( Mbid mbid ) : m_mbid( mbid )
        {}

        Album( Artist artist, QString title ) : m_artist( artist ), m_title( title )
        {}

        bool operator==( const Album& that ) const { return m_title == that.m_title && m_artist == that.m_artist; }
        bool operator!=( const Album& that ) const { return m_title != that.m_title || m_artist != that.m_artist; }
    
        operator QString() const { return title(); }
        QString title() const { return m_title.isEmpty() ? "[unknown]" : m_title; }
        Artist artist() const { return m_artist; }
        Mbid mbid() const { return m_mbid; }

        /** artist may have been set, since we allow that in the ctor, but should we handle untitled albums? */
        bool isNull() const { return m_title.isEmpty() && m_mbid.isNull(); }
    
        /** Album.getInfo WebService */
        QNetworkReply* getInfo(const QString& user = "", const QString& sk = "") const;
        QNetworkReply* share( const QStringList& recipients, const QString& message = "", bool isPublic = true ) const;

        /** use Tag::list to get the tag list out of the finished reply */
        QNetworkReply* getTags() const;
        QNetworkReply* getTopTags() const;
        
        /** Last.fm dictates that you may submit at most 10 of these */
        QNetworkReply* addTags( const QStringList& ) const;
    
        /** the Last.fm website url for this album */
        QUrl www() const;
    };
}

#endif //LASTFM_ALBUM_H
