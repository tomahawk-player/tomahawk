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

#include "TopTracksContext.h"

#include "playlist/playlistmodel.h"
#include "playlist/playlistview.h"

using namespace Tomahawk;


TopTracksContext::TopTracksContext()
    : ContextPage()
    , m_infoId( uuid() )
{
    m_topHitsView = new PlaylistView();
    m_topHitsView->setUpdatesContextView( false );
    m_topHitsModel = new PlaylistModel( m_topHitsView );
    m_topHitsModel->setStyle( TrackModel::Short );
    m_topHitsView->setTrackModel( m_topHitsModel );
    m_topHitsView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_topHitsView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    QPalette pal = m_topHitsView->palette();
    pal.setColor( QPalette::Window, QColor( 0, 0, 0, 0 ) );
    m_topHitsView->setPalette( pal );

    m_proxy = new QGraphicsProxyWidget();
    m_proxy->setWidget( m_topHitsView );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


TopTracksContext::~TopTracksContext()
{
}


void
TopTracksContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( !m_query.isNull() && query->artist() == m_query->artist() )
        return;

    m_query = query;

    Tomahawk::InfoSystem::InfoCriteriaHash artistInfo;
    artistInfo["artist"] = query->artist();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = m_infoId;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoArtistSongs;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
TopTracksContext::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_infoId )
        return;

    InfoSystem::InfoCriteriaHash trackInfo;
    trackInfo = requestData.input.value< InfoSystem::InfoCriteriaHash >();

    if ( output.canConvert< QVariantMap >() )
    {
        if ( trackInfo["artist"] != m_query->artist() )
        {
            qDebug() << "Returned info was for:" << trackInfo["artist"] << "- was looking for:" << m_query->artist();
            return;
        }
    }

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoArtistSongs:
        {
            m_topHitsModel->clear();
            const QStringList tracks = returnedData["tracks"].toStringList();

            int i = 0;
            foreach ( const QString& track, tracks )
            {
                query_ptr query = Query::get( m_query->artist(), track, QString(), uuid() );
                m_topHitsModel->append( query );

                if ( ++i == 15 )
                    break;
            }
            break;
        }

        default:
            return;
    }
}


void
TopTracksContext::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}
