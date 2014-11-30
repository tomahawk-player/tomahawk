/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright (C) 2011-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright (C) 2013, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright (C) 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright (C) 2014, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "LinkGenerator.h"

#include "TomahawkUtils.h"
#include "Logger.h"
#include "ShortLinkHelper.h"

#include "TomaHkLinkGeneratorPlugin.h"

#include "../resolvers/SyncScriptJob.h"

#include <QClipboard>
#include <QApplication>
#include <QMetaObject>
#include <memory>


using namespace Tomahawk;
using namespace Tomahawk::Utils;

LinkGenerator* LinkGenerator::s_instance = 0;


LinkGenerator*
LinkGenerator::instance()
{
    if ( !s_instance )
        s_instance = new LinkGenerator;

    return s_instance;
}


LinkGenerator::LinkGenerator( QObject* parent )
    : QObject( parent )
{
    m_defaultPlugin.reset( new TomaHkLinkGeneratorPlugin );
    m_plugins.append( m_defaultPlugin.get() );
}


LinkGenerator::~LinkGenerator()
{
}

void
LinkGenerator::copyScriptJobResultToClipboard( const QVariantMap& data )
{
    m_clipboardLongUrl = data[ "url" ].toUrl();
    copyToClipboardReady( m_clipboardLongUrl, m_clipboardLongUrl );

    sender()->deleteLater();
}


void
LinkGenerator::copyScriptJobResultToClipboardShortened( const QVariantMap& data )
{
    m_clipboardLongUrl = data[ "url" ].toUrl();

    Tomahawk::Utils::ShortLinkHelper* slh = new Tomahawk::Utils::ShortLinkHelper();
    connect( slh, SIGNAL( shortLinkReady( QUrl, QUrl, QVariant ) ),
             SLOT( copyToClipboardReady( QUrl, QUrl, QVariant ) ) );
    connect( slh, SIGNAL( done() ),
             slh, SLOT( deleteLater() ),
             Qt::QueuedConnection );
    slh->shortenLink( m_clipboardLongUrl );

    sender()->deleteLater();
}


void
LinkGenerator::copyToClipboardReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& )
{
    // Copy resulting url to clipboard
    if ( m_clipboardLongUrl == longUrl )
    {
        QClipboard* cb = QApplication::clipboard();

        QByteArray data = TomahawkUtils::percentEncode( shortUrl.isEmpty() ? longUrl : shortUrl );
        cb->setText( data );

        m_clipboardLongUrl.clear();
    }
}
