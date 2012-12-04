/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#include "CustomResultHintChecker.h"
#include "ResultHintChecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include "Query.h"
#include "utils/NetworkReply.h"
#include "utils/Closure.h"
using namespace Tomahawk;


CustomResultHintChecker::CustomResultHintChecker( const query_ptr& q )
    : ResultHintChecker( q )
    , m_query( q )
{
    if( !isValid() )
    {
        qDebug() << Q_FUNC_INFO <<  "invalid:" << url();
        return;
    }

    qDebug() << Q_FUNC_INFO << url();

    handleResultHint();
}

void
CustomResultHintChecker::handleResultHint()
{

    if ( url().startsWith( "hnhh" ) )
    {
        QUrl httpUrl = QUrl::fromUserInput( url() );
        httpUrl.setScheme( "http" );

        NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( httpUrl ) ) );

        connect( this, SIGNAL( result( result_ptr ) ), this, SLOT( handleHnhhResult( result_ptr ) ) );
        connect( reply, SIGNAL( finished() ), SLOT( hnhhFinished() ) );
        return;
    }
}

void
CustomResultHintChecker::handleHnhhResult( result_ptr result )
{
    if( !result.isNull() )
    {

        qDebug() << Q_FUNC_INFO << "Setting expiration for hotnewhiphop" << result->toString();
        qDebug() << Q_FUNC_INFO << url() << resultHint();

    }
}

void
CustomResultHintChecker::hnhhFinished()
{
    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();

    bool foundStreamable = false;
    // Intentionally accepts unknown error
    if ( r->reply()->error() != ( QNetworkReply::NoError | QNetworkReply::UnknownNetworkError ) )
    {

        QXmlStreamReader xmlStream( r->reply()->readAll() );

        if ( xmlStream.error() != QXmlStreamReader::NoError )
        {
            qDebug() << "XML ERROR!" << xmlStream.errorString();
        }
        else
        {
            while (!xmlStream.atEnd() )
            {
                xmlStream.readNext();
                if ( xmlStream.isStartElement() )
                {
                    if ( xmlStream.name().toString() == "song" )
                    {
                        const QString streamUrl = QString( xmlStream.attributes().value("filename").toLatin1() );
                        // Dont save this resulthint, it needs to revalidate
                        qDebug() << Q_FUNC_INFO << "GOT STREAMABLE " << streamUrl;
                        setResultHint( streamUrl, false );
                        foundStreamable = true;
                        break;
                    }
                }
            }
        }
    }

    if ( !foundStreamable)
    {
        removeHint();
    }

    deleteLater();
}
