/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
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
#ifndef TOMAHAWK_UTILS_LINKGENERATOR_H
#define TOMAHAWK_UTILS_LINKGENERATOR_H

#include "DllMacro.h"
#include "Typedefs.h"

namespace Tomahawk {
namespace Utils {

class ShortLinkHelperPrivate;

class DLLEXPORT LinkGenerator : public QObject
{
    Q_OBJECT
public:
    static LinkGenerator* instance();
    virtual ~LinkGenerator();

    QUrl openLinkFromQuery( const Tomahawk::query_ptr& query ) const;

    QUrl copyOpenLink( const Tomahawk::artist_ptr& artist ) const;
    QUrl copyOpenLink( const Tomahawk::album_ptr& album ) const;

    QUrl openLink( const QString& title, const QString& artist, const QString& album ) const;

    QString copyPlaylistToClipboard( const Tomahawk::dynplaylist_ptr& playlist );


public slots:
    /// Creates a link from the requested data and copies it to the clipboard
    void copyToClipboard( const Tomahawk::query_ptr& query );

private slots:
    void copyToClipboardReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& callbackObj );

private:
    explicit LinkGenerator( QObject* parent = 0 );
    QString hostname() const;

    QUrl m_clipboardLongUrl;

    static LinkGenerator* s_instance;
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_LINKGENERATOR_H
