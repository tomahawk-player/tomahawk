/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#include "dropjobnotifier.h"

#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"
#include "sourcelist.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "dropjob.h"
#include <qjson/parser.h>
#include "dropjobnotifier.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class QNetworkReply;

using namespace Tomahawk;

DropJobNotifier::DropJobNotifier( QPixmap servicePixmap, QString service, DropJob::DropType type, QNetworkReply* job )
    : JobStatusItem()
    , m_type( "unknown" )
    , m_job( 0 )
    , m_pixmap ( servicePixmap )
    , m_service ( service )
{

    if( type == DropJob::Playlist )
        m_type = "playlist";

    if( type == DropJob::Artist )
        m_type = "artist";

    if( type == DropJob::Track )
        m_type = "track";

    if( type == DropJob::Album )
        m_type = "album";

    if( m_service.isEmpty() )
        m_service = "DropJob";

    connect( job, SIGNAL( finished() ), this, SLOT( setFinished() ) );
}


DropJobNotifier::~DropJobNotifier()
{}

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
    return tr( "Parsing %1 %2" ).arg( m_service )
                                .arg( m_type );
}

void
DropJobNotifier::setFinished()
{
    emit finished();
}

