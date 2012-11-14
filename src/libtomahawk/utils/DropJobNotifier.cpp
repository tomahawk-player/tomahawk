/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DropJobNotifier.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <qjson/parser.h>

#include "Query.h"
#include "SourceList.h"
#include "DropJob.h"
#include "DropJobNotifier.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DropJobNotifier::DropJobNotifier( QPixmap servicePixmap, QString service, DropJob::DropType type, NetworkReply* job )
    : JobStatusItem()
    , m_type( "unknown" )
    , m_job( 0 )
    , m_pixmap( servicePixmap )
    , m_service( service )
{
    init( type );

    if ( m_service.isEmpty() )
        m_service = "DropJob";

    connect( job, SIGNAL( finished() ), this, SLOT( setFinished() ) );
}


DropJobNotifier::DropJobNotifier( QPixmap pixmap, DropJob::DropType type )
    : JobStatusItem()
    , m_job( 0 )
    , m_pixmap( pixmap )
{
    init( type );
}


DropJobNotifier::~DropJobNotifier()
{
}


void
DropJobNotifier::init( DropJob::DropType type )
{
    if ( type == DropJob::Playlist )
        m_type = tr( "playlist" );

    if ( type == DropJob::Artist )
        m_type = tr( "artist" );

    if ( type == DropJob::Track )
        m_type = tr( "track" );

    if ( type == DropJob::Album )
        m_type = tr( "album" );
}


QString
DropJobNotifier::rightColumnText() const
{
    return QString();
}


QPixmap
DropJobNotifier::icon() const
{
    return m_pixmap;
}


QString
DropJobNotifier::mainText() const
{
    if ( m_service.isEmpty() )
    {
        return tr( "Fetching %1 from database" ).arg( m_type );
    }
    else
    {
        return tr( "Parsing %1 %2" ).arg( m_service )
                                    .arg( m_type );
    }
}


void
DropJobNotifier::setFinished()
{
    emit finished();
}

