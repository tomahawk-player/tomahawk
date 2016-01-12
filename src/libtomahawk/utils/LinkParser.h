/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2014  Dominik Schmidt <domme@tomahawk-player.org>
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

#pragma once
#ifndef TOMAHAWK_UTILS_LINKPARSER_H
#define TOMAHAWK_UTILS_LINKPARSER_H

#include "../resolvers/ScriptJob.h"
#include "LinkParserPlugin.h"
#include "UrlType.h"

#include "../DllMacro.h"
#include "../Typedefs.h"

#include <memory>

namespace Tomahawk {

namespace Utils {

class LinkParserPlugin;

class DLLEXPORT LinkParser : public QObject
{
    Q_OBJECT
public:
    static LinkParser* instance();
    virtual ~LinkParser();

    void addPlugin( const QSharedPointer< LinkParserPlugin >& plugin );
    void removePlugin( const QSharedPointer< LinkParserPlugin >& plugin );

    bool canParseUrl( const QString& url, UrlType type ) const;
    QList< QSharedPointer < LinkParserPlugin > > parserPluginsForUrl( const QString& url, UrlType type ) const;
    void lookupUrl( const QString& url, const QList< QSharedPointer < LinkParserPlugin > >& parserPlugins = QList< QSharedPointer < LinkParserPlugin > >() ) const;

signals:
    void informationFound( const QString& url, const QSharedPointer<QObject>& information );

private slots:
    void  onInformationFound( const QString& url, const QSharedPointer<QObject>& information );

private:
    explicit LinkParser( QObject* parent = 0 );
    QList< QSharedPointer < LinkParserPlugin > > m_plugins;


    static LinkParser* s_instance;
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_LINKPARSER_H
