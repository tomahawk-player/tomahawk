/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef TOMAHAWKOAUTHTWITTERACCOUNT
#define TOMAHAWKOAUTHTWITTERACCOUNT

#include "accounts/AccountDllMacro.h"
#include "utils/TomahawkUtils.h"

#include <QTweetLib/qtweetlib_global.h>
#include <QTweetLib/oauthtwitter.h>

class ACCOUNTDLLEXPORT TomahawkOAuthTwitter : public OAuthTwitter
{
    Q_OBJECT

public:
    TomahawkOAuthTwitter( QNetworkAccessManager *nam = TomahawkUtils::nam() , QObject *parent = 0 );

    ~TomahawkOAuthTwitter() {}

protected:
    virtual const QString authorizationWidget();

private slots:
    void error();
};

#endif
