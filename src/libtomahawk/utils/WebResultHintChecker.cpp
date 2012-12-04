/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#include "WebResultHintChecker.h"
#include "ResultHintChecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include "Query.h"
#include "utils/NetworkReply.h"

using namespace Tomahawk;


WebResultHintChecker::WebResultHintChecker( const query_ptr& q )
    : ResultHintChecker( q )
{
    if ( !isValid() )
    {
        return;
    }

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->head( QNetworkRequest( QUrl::fromUserInput( url() ) ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( headFinished() ) );
}

void
WebResultHintChecker::headFinished()
{
    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();

    if ( r->reply()->error() != QNetworkReply::NoError )
    {
        // Error getting headers for the http resulthint, remove it from the result
        // as it's definitely not playable
        removeHint();
    }

    deleteLater();
}
