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
#include "LinkGeneratorPlugin.h"

#include "../DllMacro.h"
#include "../Typedefs.h"

#include <memory>

namespace Tomahawk {

namespace Utils {

class DLLEXPORT LinkGenerator : public QObject
{
    Q_OBJECT
public:
    static LinkGenerator* instance();
    virtual ~LinkGenerator();

    // TODO: openLink(QString, QString, QString) is a rather annoying special case. Can we get rid of it?

    ScriptJob* openLink( const QString& title, const QString& artist, const QString& album ) const
    {
        ScriptJob* job;
        QList< LinkGeneratorPlugin* >::const_iterator i = m_plugins.constEnd();
        while ( i != m_plugins.constBegin() )
        {
            --i;
            job = (*i)->openLink( title, artist, album );
            if ( job )
            {
                break;
            }
        }

        // No suitable link generator plugin found
        Q_ASSERT( job );
        return job;
    }

    template <typename T> ScriptJob* openLink( const T& item ) const
    {
        ScriptJob* job;
        QList< LinkGeneratorPlugin* >::const_iterator i = m_plugins.constEnd();
        while ( i != m_plugins.constBegin() )
        {
            --i;
            job = (*i)->openLink( item );
            if ( job )
            {
                break;
            }
        }

        // No suitable link generator plugin found
        Q_ASSERT( job );
        return job;
    }

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

    QUrl m_clipboardLongUrl;

    std::unique_ptr< LinkGeneratorPlugin > m_defaultPlugin;
    QList< LinkGeneratorPlugin* > m_plugins;


    static LinkGenerator* s_instance;
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_LINKGENERATOR_H
