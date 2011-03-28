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
    
        static RadioStation library( const lastfm::User& user )         { return rql( libraryStr( user ) ); }
        static RadioStation recommendations( const lastfm::User& user ) { return rql( recommendationsStr( user ) ); }
        static RadioStation neighbourhood( const lastfm::User& user )   { return rql( neighbourhoodStr( user ) ); }
        static RadioStation lovedTracks( const lastfm::User& user )     { return rql( lovedTracksStr( user ) ); }
        static RadioStation globalTag( const lastfm::Tag& tag )         { return rql( globalTagStr( tag ) ); }
        static RadioStation similar( const lastfm::Artist& artist )     { return rql( similarStr( artist ) ); }
        static RadioStation userTag( const lastfm::User& user, const lastfm::Tag& tag) { return rql( userTagStr( user, tag ) ); }
        static RadioStation playlist( int playlistId )                  { return rql( playlistStr( playlistId ) ); }
        static RadioStation adventure( const lastfm::User& user )       { return rql( adventureStr( user ) ); }

        static RadioStation rql( const QString& rql )
        {
            RadioStation station;
            station.setRql( rql );
            return station;
        }

        /** eg. "mxcl's Loved Tracks"
          * It is worth noting that the Radio doesn't set the title of RadioStation 
          * object until we have tuned to it, and then we only set the one we give 
          * you back.
          */    
        QString title() const { return m_title; }
        /** the Last.fm url, eg. lastfm://user/mxcl/loved */
        QString url() const { return m_url; }
        QString rql() const { return m_rql; }

        void setTitle( const QString& s ) { m_title = s; }

        bool setRep(float rep);
        bool setMainstr(float mainstr);
        bool setDisco(bool disco);

        float rep() const;
        float mainstr() const;
        bool disco() const;

        bool isLegacyPlaylist() const
        {
            return m_url.startsWith( "lastfm://play/" ) ||
                   m_url.startsWith( "lastfm://preview/" ) ||
                   m_url.startsWith( "lastfm://track/" ) ||
                   m_url.startsWith( "lastfm://playlist/" );
        }

        // good for getRecentStations:
        static QList<RadioStation> list( QNetworkReply* );

    private:
        void setRql( const QString& rql )
        {
            m_rql = rql;
            m_url = "lastfm://rql/" + QString(rql.toUtf8().toBase64());
        }

        void setString( const QString& s );

        static QString libraryStr( const lastfm::User& user )         { return "library:" + user ; }
        static QString recommendationsStr( const lastfm::User& user ) { return "rec:" + user ; }
        static QString neighbourhoodStr( const lastfm::User& user )   { return "neigh:" + user ; }
        static QString lovedTracksStr( const lastfm::User& user )     { return "loved:" + user ; }
        static QString globalTagStr( const lastfm::Tag& tag )         { return "tag:\"" + tag + "\"" ; }
        static QString similarStr( const lastfm::Artist& artist )     { return "simart:\"" + artist + "\""; }
        static QString userTagStr( const lastfm::User& user, const lastfm::Tag& tag) { return "ptag:\"" + tag + "\"|" + user ; }
        static QString playlistStr( int playlistId )                  { return "playlist:" + QString::number(playlistId) ; }
        static QString adventureStr( const lastfm::User& user )       { return "adv:" + user ; } 
    private:
        QString m_rql;
        QString m_url;
        QString m_title;
    };
}


Q_DECLARE_METATYPE( lastfm::RadioStation )


inline QDebug operator<<( QDebug d, const lastfm::RadioStation& station )
{
    return d << station.url();
}

#endif
