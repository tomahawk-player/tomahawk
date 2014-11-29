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

#include "../resolvers/ScriptJob.h"

#include "../DllMacro.h"
#include "../Typedefs.h"

namespace Tomahawk {

namespace Utils {

class DLLEXPORT LinkGenerator : public QObject
{
    Q_OBJECT
public:
    static LinkGenerator* instance();
    virtual ~LinkGenerator();

    ScriptJob* openLink( const QString& title, const QString& artist, const QString& album ) const;
    ScriptJob* openLink( const Tomahawk::query_ptr& query ) const;
    ScriptJob* openLink( const Tomahawk::artist_ptr& artist ) const;
    ScriptJob* openLink( const Tomahawk::album_ptr& album ) const;
    ScriptJob* openLink( const Tomahawk::dynplaylist_ptr& playlist ) const;

    // Fire and forget

    // the query link is shortened automatically
    void copyOpenLink( const query_ptr& query ) const
    {
        ScriptJob* job = openLink( query );
        connect( job, SIGNAL( done( QVariantMap ) ), SLOT( copyScriptJobResultToClipboardShortened( QVariantMap ) ), Qt::QueuedConnection );
        job->start();
    }

    // all others are not
    template <typename T> void copyOpenLink( const T& item ) const
    {
        ScriptJob* job = openLink( item );
        connect( job, SIGNAL( done( QVariantMap ) ), SLOT( copyScriptJobResultToClipboard( QVariantMap ) ), Qt::QueuedConnection );
        job->start();
    }

private slots:
    void copyToClipboardReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& callbackObj = QVariant() );
    void copyScriptJobResultToClipboard( const QVariantMap& data );
    void copyScriptJobResultToClipboardShortened( const QVariantMap& data );

private:
    explicit LinkGenerator( QObject* parent = 0 );
    QString hostname() const;

    QUrl m_clipboardLongUrl;

    static LinkGenerator* s_instance;
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_LINKGENERATOR_H
