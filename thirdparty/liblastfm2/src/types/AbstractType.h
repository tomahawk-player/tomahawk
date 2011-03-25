/*
   Copyright 2010 Last.fm Ltd.
      - Primarily authored by Micahel Coffey and Jono Cole

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

#ifndef ABSTRACTTYPE_H
#define ABSTRACTTYPE_H

#include <QDomElement>
#include <QString>
#include <QUrl>

#include <lastfm/global.h>

namespace lastfm
{
    class LASTFM_DLLEXPORT AbstractType
    {
    public:
        virtual QString toString() const = 0;
        virtual QDomElement toDomElement( QDomDocument& ) const = 0;
        virtual QUrl www() const = 0;
        virtual QUrl imageUrl( ImageSize size, bool square ) const = 0;
        virtual ~AbstractType() {;}
    };
};

#endif // ABSTRACTTYPE_H
