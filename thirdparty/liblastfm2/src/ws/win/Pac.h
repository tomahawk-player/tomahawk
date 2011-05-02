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
#ifndef WS_AUTOPROXY_H
#define WS_AUTOPROXY_H

#include <QNetworkProxy>
#include <windows.h>
#include <winhttp.h>
class QNetworkRequest;

namespace lastfm
{
    /** @brief simple wrapper to do per url automatic proxy detection
      * @author <doug@last.fm>
      */
    class Pac
    {
        HINTERNET m_hSession;
        bool m_bFailed;

    public:
        Pac();
        ~Pac();

        QNetworkProxy resolve( const QNetworkRequest& url, const wchar_t* pacUrl );

        void resetFailedState() { m_bFailed = false; }

    private:
        Pac( const Pac& ); //undefined
        Pac operator=( const Pac& ); //undefined
    };
}

#endif