/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2016, Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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

#include "LinkParser.h"

#include "TomahawkUtils.h"
#include "Logger.h"
#include "../resolvers/SyncScriptJob.h"

using namespace Tomahawk;
using namespace Tomahawk::Utils;

LinkParser* LinkParser::s_instance = 0;


LinkParser*
LinkParser::instance()
{
    if ( !s_instance )
        s_instance = new LinkParser;

    return s_instance;
}


LinkParser::LinkParser( QObject* parent )
    : QObject( parent )
{
}


LinkParser::~LinkParser()
{
}


void LinkParser::addPlugin( const QSharedPointer< LinkParserPlugin >& plugin )
{
    m_plugins.append( plugin );
    connect( plugin.data(), SIGNAL( informationFound( QString, QSharedPointer<QObject> ) ), SLOT( onInformationFound( QString, QSharedPointer<QObject> ) ) );
}


void
LinkParser::removePlugin( const QSharedPointer< LinkParserPlugin >& plugin )
{
    if ( !plugin.isNull() )
    {
        disconnect( plugin.data(), 0, this, 0);

    }

    QMutableListIterator< QSharedPointer< LinkParserPlugin > > iter( m_plugins );
    while ( iter.hasNext() )
    {
        QSharedPointer< LinkParserPlugin > ptr = iter.next();
        if ( ptr.data() == plugin.data() || ptr.isNull() )
        {
            iter.remove();
        }
    }
}


bool
LinkParser::canParseUrl( const QString& url, UrlType type ) const
{
    return !parserPluginsForUrl( url, type).isEmpty();
}


QList< QSharedPointer< LinkParserPlugin > >
LinkParser::parserPluginsForUrl( const QString& url, Tomahawk::Utils::UrlType type ) const
{
    QList< QSharedPointer< LinkParserPlugin > > plugins;

    foreach ( const QSharedPointer< LinkParserPlugin >& plugin, m_plugins )
    {
        if ( plugin->canParseUrl( url, type ) )
        {
            plugins.append( plugin );
        }
    }

    return plugins;
}


void
LinkParser::lookupUrl( const QString& url, const QList< QSharedPointer < LinkParserPlugin > >& parserPlugins ) const
{
    foreach ( const QSharedPointer< LinkParserPlugin >& plugin, parserPlugins )
    {
        if ( !plugin.isNull() )
        {
            plugin->lookupUrl( url );
        }
    }
}


void
LinkParser::onInformationFound( const QString& url, const QSharedPointer<QObject>& information )
{
    tLog() << Q_FUNC_INFO << url;
    emit informationFound( url, information );
}
