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
#include "Mbid.h"
#include "mbid_mp3.c"
#include <QFile>

namespace lastfm
{
    Mbid //static
    Mbid::fromLocalFile( const QString& path )
    {   
        char out[MBID_BUFFER_SIZE];
        QByteArray const bytes = QFile::encodeName( path );
        int const r = getMP3_MBID( bytes.data(), out );
        Mbid mbid;
        if (r == 0) mbid.id = QString::fromLatin1( out );
        return mbid;
    }
}
