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

#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "DropJobNotifier.h"
#include "Query.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "Source.h"

#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

QPixmap* ShortenedLinkParser::s_pixmap = 0;

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
             url.contains( "rd.io" ) );
}


void
ShortenedLinkParser::lookupUrl( const QString& url )
{
    tDebug() << "Looking up..." << url;
    QString cleaned = url;
    if ( cleaned.contains( "/#/s/" ) )
        cleaned.replace( "/#", "" );

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( QUrl( cleaned ) ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( lookupFinished() ) );

    m_queries.insert( reply );

    m_expandJob = new DropJobNotifier( pixmap(), "shortened", DropJob::Track, reply );
    JobStatusView::instance()->model()->addJob( m_expandJob );

}


void
ShortenedLinkParser::lookupFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    if ( r->error() != QNetworkReply::NoError )
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Network error parsing shortened link!" ) ) );

    QVariant redir = r->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if ( redir.isValid() && !redir.toUrl().isEmpty() )
    {
        tDebug() << "RedirectionTargetAttribute set on " << redir;
        m_queries.remove( r );
        r->deleteLater();
        lookupUrl( redir.toUrl().toString() );
    }
    else
    {
        tLog() << "Got a redirected url:" << r->url().toString();
        m_links << r->url().toString();
        m_queries.remove( r );
        r->deleteLater();
        checkFinished();
    }
}


void
ShortenedLinkParser::checkFinished()
{
    if ( m_queries.isEmpty() ) // we're done
    {
        qDebug() << "DONE and found redirected urls:" << m_links;
        emit urls( m_links );

        deleteLater();
    }
}


#ifndef ENABLE_HEADLESS

QPixmap
ShortenedLinkParser::pixmap()
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/add.png" );

    return *s_pixmap;
}

#endif
