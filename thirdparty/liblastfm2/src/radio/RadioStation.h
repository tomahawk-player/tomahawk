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
#ifndef LASTFM_RADIO_STATION_H
#define LASTFM_RADIO_STATION_H

#include <lastfm/User>
#include <lastfm/Tag>
#include <lastfm/Artist>

namespace lastfm
{
    /** @author <jono@last.fm> 
      */
    class LASTFM_DLLEXPORT RadioStation
    {
    public:
        RadioStation()
        {}
        RadioStation( const QString& s )
        {
        setString( s );
        }
    
        static RadioStation library( const lastfm::User& user );
        static RadioStation library( QList<lastfm::User>& users );

        static RadioStation similar( const lastfm::Artist& artist );
        static RadioStation similar( QList<lastfm::Artist>& artist );

        static RadioStation tag( const lastfm::Tag& tag );
        static RadioStation tag( QList<lastfm::Tag>& tag );

        static RadioStation recommendations( const lastfm::User& user );
        static RadioStation friends( const lastfm::User& user );
        static RadioStation neighbourhood( const lastfm::User& user );

        static RadioStation mix( const lastfm::User& user );

        QNetworkReply* getSampleArtists( int limit = 50 ) const;
        QNetworkReply* getTagSuggestions( int limit = 50 ) const;

        /** eg. "mxcl's Loved Tracks"
          * It is worth noting that the Radio doesn't set the title of RadioStation 
          * object until we have tuned to it, and then we only set the one we give 
          * you back.
          */    
        QString title() const;
        /** the Last.fm url, eg. lastfm://user/mxcl/loved */
        QString url() const;

        void setTitle( const QString& title );

        void setTagFilter( const QString& tag );

        void setRep(float rep);
        void setMainstr(float mainstr);
        void setDisco(bool disco);

        float rep() const;
        float mainstr() const;
        bool disco() const;

        bool isLegacyPlaylist() const
        {
            return m_url.toString().startsWith( "lastfm://play/" ) ||
                   m_url.toString().startsWith( "lastfm://preview/" ) ||
                   m_url.toString().startsWith( "lastfm://track/" ) ||
                   m_url.toString().startsWith( "lastfm://playlist/" );
        }

        // good for getRecentStations:
        static QList<RadioStation> list( QNetworkReply* );

        bool operator==( const RadioStation& that ) const;

    private:
        void setString( const QString& s );

        static QString libraryStr( QList<lastfm::User>& user );
        static QString recommendationsStr( const lastfm::User& user ) { return "lastfm://user/" + user + "/recommended"; }
        static QString friendsStr( const lastfm::User& user )         { return "lastfm://user/" + user + "/friends"; }
        static QString neighbourhoodStr( const lastfm::User& user )   { return "lastfm://user/" + user + "/neighbours"; }
        static QString tagStr( QList<lastfm::Tag>& tag );
        static QString similarStr( QList<lastfm::Artist>& artist );
        static QString mixStr( const lastfm::User& user )             { return "lastfm://user/" + user + "/mix"; }
    private:
        QUrl m_url;
        QString m_title;
        QString m_tagFilter;

        float m_rep;
        float m_mainstr;
        bool m_disco;
    };
}


Q_DECLARE_METATYPE( lastfm::RadioStation )


inline QDebug operator<<( QDebug d, const lastfm::RadioStation& station )
{
    return d << station.url();
}

#endif
