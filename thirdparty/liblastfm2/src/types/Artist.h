/*
   Copyright 2009-2010 Last.fm Ltd.
      - Primarily authored by Max Howell, Jono Cole, Doug Mansell and Michael Coffey

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
#ifndef LASTFM_ARTIST_H
#define LASTFM_ARTIST_H

#include <QMap>
#include <QString>
#include <QUrl>

#include <lastfm/AbstractType>
#include <lastfm/global.h>


namespace lastfm
{
    class LASTFM_DLLEXPORT Artist : public AbstractType
    {
    private:
        QString m_name;
        QList<QUrl> m_images;

    public:
        Artist() : AbstractType()
        {}

        Artist( const QString& name ) : AbstractType(), m_name( name )
        {}

        Artist( const class XmlQuery& xml );

        /** will be QUrl() unless you got this back from a getInfo or something call */
        QUrl imageUrl( ImageSize size = Large, bool square = false ) const;

        bool isNull() const { return m_name.isEmpty(); }

        /** the url for this artist's page at www.last.fm */
        QUrl www() const;

        Artist& operator=( const Artist& that ) { m_name = that.name(); m_images = that.m_images; return *this; }
        bool operator==( const Artist& that ) const { return m_name == that.m_name; }
        bool operator!=( const Artist& that ) const { return m_name != that.m_name; }
        bool operator<( const Artist& that ) const { return m_name < that.m_name; }

        operator QString() const
        {
            /** if no artist name is set, return the musicbrainz unknown identifier
              * in case some part of the GUI tries to display it anyway. Note isNull
              * returns false still. So you should have queried that! */
            return m_name.isEmpty() ? "[unknown]" : m_name;
        }

        QString toString() const { return name(); }
        QString name() const { return QString(*this); }

        QDomElement toDomElement( QDomDocument& ) const { return QDomElement(); }

        QNetworkReply* share( const QStringList& recipients, const QString& message = "", bool isPublic = true ) const;

        QNetworkReply* getEvents(int limit = 0) const;
        QNetworkReply* getInfo() const;
        static Artist getInfo( QNetworkReply* );

        QNetworkReply* getSimilar( int limit = -1 ) const;
        /** The match percentage is returned from last.fm as a 4 significant
          * figure floating point value. So we multply it by 100 to make an
          * integer in the range of 0 to 10,000. This is possible confusing
          * for you, but I felt it best not to lose any precision, and floats
          * aren't much fun. */
        static QMap<int, QString> getSimilar( QNetworkReply* );

        /** use Tag::list to get the tag list out of the finished reply */
        QNetworkReply* getTags() const;
        QNetworkReply* getTopTags() const;

        QNetworkReply* getTopTracks() const;
        static QStringList getTopTracks( QNetworkReply* );

        /** Last.fm dictates that you may submit at most 10 of these */
        QNetworkReply* addTags( const QStringList& ) const;

        QNetworkReply* search( int limit = -1 ) const;
        static QList<Artist> list( QNetworkReply* );

        QMap<QString, QString> params( const QString& method ) const;
    };
}

#endif
