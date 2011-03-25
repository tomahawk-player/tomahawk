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

#ifndef LASTFM_GLOBAL_H
#define LASTFM_GLOBAL_H

#define LASTFM_VERSION 0x00000400
#define LASTFM_VERSION_STRING "0.4.0"
#define LASTFM_MAJOR_VERSION 0
#define LASTFM_MINOR_VERSION 4
#define LASTFM_PATCH_VERSION 0


#include <QtGlobal>

#ifdef Q_CC_MSVC
    #ifdef LASTFM_LIB
        #define LASTFM_DLLEXPORT __declspec(dllexport)
    #else
        #define LASTFM_DLLEXPORT __declspec(dllimport)
    #endif
	#ifdef LASTFM_FINGERPRINT_LIB
        #define LASTFM_FINGERPRINT_DLLEXPORT __declspec(dllexport)
    #else
        #define LASTFM_FINGERPRINT_DLLEXPORT __declspec(dllimport)
    #endif
#elif __GNUC__ >= 4
    #define LASTFM_DLLEXPORT __attribute__ ((visibility("default")))
	#define LASTFM_FINGERPRINT_DLLEXPORT __attribute__ ((visibility("default")))
#else
    #define LASTFM_DLLEXPORT
	#define LASTFM_FINGERPRINT_DLLEXPORT
#endif



#include <QMetaEnum>
#include <QString>

namespace lastfm
{
    /** http://labs.trolltech.com/blogs/2008/10/09/coding-tip-pretty-printing-enum-values
      * Tips for making this take a single parameter welcome! :)
      * 
      * eg. lastfm::qMetaEnumString<QNetworkReply>( error, "NetworkError" );
      */
    template <typename T> static inline QString qMetaEnumString( int enum_value, const char* enum_name )
    {
        QMetaObject meta = T::staticMetaObject;
        for (int i=0; i < meta.enumeratorCount(); ++i)
        {
            QMetaEnum m = meta.enumerator(i);
            if (m.name() == QLatin1String(enum_name))
                return QLatin1String(m.valueToKey(enum_value));
        }
        return QString("Unknown enum value for \"%1\": %2").arg( enum_name ).arg( enum_value );
    }


    enum ImageSize
    {
        Small,
        Medium,
        Large, /** seemingly 174x174 */
        ExtraLarge,
        Mega
    };
    
    
    //convenience
    class Album;
    class Artist;
    class Audioscrobbler;
    class AuthenticatedUser;
    class Fingerprint;
    class FingerprintableSource;
    class FingerprintId;
    class Mbid;
    class MutableTrack;
    class NetworkAccessManager;
    class Playlist;
    class User;
    class RadioStation;
    class Tag;
    class Track;
    class XmlQuery;
    class Xspf;
}


#ifdef LASTFM_COLLAPSE_NAMESPACE
using lastfm::Album;
using lastfm::Artist;
using lastfm::Audioscrobbler;
using lastfm::AuthenticatedUser;
using lastfm::Fingerprint;
using lastfm::FingerprintId;
using lastfm::Mbid;
using lastfm::MutableTrack;
using lastfm::Playlist;
using lastfm::User;
using lastfm::RadioStation;
using lastfm::Tag;
using lastfm::Track;
using lastfm::XmlQuery;
using lastfm::Xspf;
#endif


//convenience
class QDomDocument;
class QNetworkAccessManager;
class QNetworkReply;


//convenience for development
#include <QDebug>

#endif //LASTFM_GLOBAL_H
