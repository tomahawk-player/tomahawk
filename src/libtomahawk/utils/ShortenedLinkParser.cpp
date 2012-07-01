/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ShortenedLinkParser.h"

#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>

#include "DropJobNotifier.h"
#include "Query.h"
#include "Source.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ShortenedLinkParser::ShortenedLinkParser ( const QStringList& urls, QObject* parent )
    : QObject( parent )
{
    foreach ( const QString& url, urls )
        if ( handlesUrl( url ) )
            lookupUrl( url ) ;
}


ShortenedLinkParser::~ShortenedLinkParser()
{
}


bool
ShortenedLinkParser::handlesUrl( const QString& url )
{
    // Whitelisted links
    return ( url.contains( "t.co" ) ||
             url.contains( "bit.ly" ) ||
             url.contains( "j.mp" ) ||
             url.contains( "spoti.fi" ) ||
             url.contains( "ow.ly" ) ||
             url.contains( "fb.me" ) ||
             url.contains( "itun.es" ) ||
             url.contains( "tinyurl.com" ) ||
             url.contains( "tinysong.com" ) ||
             url.contains( "grooveshark.com/s/~/" ) || // These redirect to the 'real' grooveshark track url
             url.contains( "grooveshark.com/#/s/~/" ) ||
             url.contains( "rd.io" ) ||
             url.contains( "snd.sc" ) );
}


void
ShortenedLinkParser::lookupUrl( const QString& url )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Looking up..." << url;
    QString cleaned = url;
    if ( cleaned.contains( "/#/s/" ) )
        cleaned.replace( "/#", "" );

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( QUrl( cleaned ) ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( lookupFinished() ) );

    m_queries.insert( reply );

#ifndef ENABLE_HEADLESS
    m_expandJob = new DropJobNotifier( pixmap(), "shortened", DropJob::Track, reply );
    JobStatusView::instance()->model()->addJob( m_expandJob );
#endif
}


void
ShortenedLinkParser::lookupFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );

#ifndef ENABLE_HEADLESS
    if ( r->reply()->error() != QNetworkReply::NoError )
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Network error parsing shortened link!" ) ) );
#endif

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Got an un-shortened url:" << r->reply()->url().toString();
    m_links << r->reply()->url().toString();
    m_queries.remove( r );
    r->deleteLater();

    checkFinished();
}


void
ShortenedLinkParser::checkFinished()
{
    if ( m_queries.isEmpty() ) // we're done
    {
        emit urls( m_links );

        deleteLater();
    }
}


#ifndef ENABLE_HEADLESS

QPixmap
ShortenedLinkParser::pixmap()
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Add );
}

#endif
