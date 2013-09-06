/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "NetworkReply.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/NetworkAccessManager.h"

#include <QNetworkAccessManager>

using namespace Tomahawk;


NetworkReply::NetworkReply( QNetworkReply* parent )
    : QObject()
    , m_reply( parent )
{
    m_url = m_reply->url();

    connectReplySignals();
}


NetworkReply::~NetworkReply()
{
    if ( m_reply )
        m_reply->deleteLater();
}


void
NetworkReply::blacklistHostFromRedirection( const QString& host )
{
    m_blacklistedHosts << host;
}


void
NetworkReply::deletedByParent()
{
    if ( sender() == m_reply )
    {
        m_reply = 0;
        deleteLater();
    }
}


void
NetworkReply::metaDataChanged()
{
    // Do an early check if we are redirected.
    QVariant redir = m_reply->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if ( redir.isValid() && !redir.toUrl().isEmpty() )
    {
        // We have found a redirect, so follow it.
        if ( m_formerUrls.count( redir.toUrl().toString() ) < maxSameRedirects && m_formerUrls.count() < maxRedirects )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Redirected HTTP request to" << redir;
            if ( m_blacklistedHosts.contains( redir.toUrl().host() ) )
            {
                // Nope, we won't follow this redirect, emit some other signal here.
                emit finalUrlReached();
                emit finalUrlReached( redir.toUrl() );
            }
            else
            {
                disconnectReplySignals();
                load( redir.toUrl() );
                emit redirected();
            }
        }
        else
        {
            // Nope, we won't follow this redirect, emit some other signal here.
            emit finalUrlReached();
            emit finalUrlReached( m_url );
        }
    }
    else
    {
        // We do not have enough information to handle a redirect but if this isn't a redirect, we're maybe already done.
        QVariant statusCode = m_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
        if ( statusCode.isValid() && statusCode.toInt() >= 200 && statusCode.toInt() <= 207 )
        {
            emit finalUrlReached();
            emit finalUrlReached( m_url );
        }
    }
}


void
NetworkReply::connectReplySignals()
{
    connect( m_reply, SIGNAL( finished() ), SLOT( networkLoadFinished() ) );
    connect( m_reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SIGNAL( error( QNetworkReply::NetworkError ) ) );
    connect( m_reply, SIGNAL( destroyed( QObject* ) ), SLOT( deletedByParent() ) );
    connect( m_reply, SIGNAL( metaDataChanged() ), SLOT( metaDataChanged() ) );
}


void
NetworkReply::disconnectReplySignals()
{
    disconnect( m_reply, SIGNAL( finished() ), this, SLOT( networkLoadFinished() ) );
    disconnect( m_reply, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SIGNAL( error( QNetworkReply::NetworkError ) ) );
    disconnect( m_reply, SIGNAL( destroyed( QObject* ) ), this, SLOT( deletedByParent() ) );
    disconnect( m_reply, SIGNAL( metaDataChanged() ), this, SLOT( metaDataChanged() ) );
}


void
NetworkReply::load( const QUrl& url )
{
    m_url = url;
    m_formerUrls << url.toString();
    QNetworkRequest request( url );

    Q_ASSERT( Tomahawk::Utils::nam() != 0 );

    QNetworkAccessManager::Operation op = m_reply->operation();
    m_reply->deleteLater();

    switch ( op )
    {
        case QNetworkAccessManager::HeadOperation:
            m_reply = Tomahawk::Utils::nam()->head( request );
            break;

        default:
            m_reply = Tomahawk::Utils::nam()->get( request );
    }

    connectReplySignals();
}


void
NetworkReply::emitFinished( const QUrl& url )
{
    emit finalUrlReached();
    emit finalUrlReached( url );
    emit finished( url );
    emit finished();
}


void
NetworkReply::networkLoadFinished()
{
    if ( m_reply->error() != QNetworkReply::NoError )
    {
        emitFinished( m_url );
        return;
    }

    QVariant redir = m_reply->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if ( redir.isValid() && !redir.toUrl().isEmpty() && m_formerUrls.count( redir.toUrl().toString() ) < maxSameRedirects && m_formerUrls.count() < maxRedirects )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Redirected HTTP request to" << redir;
        if ( m_blacklistedHosts.contains( redir.toUrl().host() ) )
        {
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Reached blacklisted host, not redirecting anymore.";
            emitFinished( redir.toUrl() );
        }
        else
        {
            load( redir.toUrl() );
            emit redirected();
        }
    }
    else
    {
        emitFinished( m_url );
    }
}
