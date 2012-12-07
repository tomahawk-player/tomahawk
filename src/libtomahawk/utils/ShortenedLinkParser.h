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

#ifndef SNORTENED_LINK_PARSER_H
#define SNORTENED_LINK_PARSER_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QObject>
#include <QSet>
#include <QStringList>

#ifndef ENABLE_HEADLESS
    #include <QPixmap>
#endif

class NetworkReply;

namespace Tomahawk
{

class DropJobNotifier;

/**
 * Small class to parse whitelisted shortened links into the redirected urls
 *
 * Connect to urls() to get the result
 *
 */
class DLLEXPORT ShortenedLinkParser : public QObject
{
    Q_OBJECT

public:
    explicit ShortenedLinkParser( const QStringList& urls, QObject* parent = 0 );
    virtual ~ShortenedLinkParser();

    static bool handlesUrl( const QString& url );

public slots:
    void lookupFinished();

signals:
    void urls( const QStringList& urls );

private:
    void lookupUrl( const QString& url );
    void checkFinished();

#ifndef ENABLE_HEADLESS
    static QPixmap pixmap();
#endif

    QStringList m_links;
    QSet< NetworkReply* > m_queries;
    DropJobNotifier* m_expandJob;
};

}

#endif
