/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef EXTERNALRESOLVER_H
#define EXTERNALRESOLVER_H

#include "Source.h"
#include "DllMacro.h"
#include "Resolver.h"
#include "ScriptCommandQueue.h"
#include "ScriptCommand_LookupUrl.h"
#include "Typedefs.h"

#include <QObject>

class QWidget;

namespace Tomahawk
{

/**
 * Generic resolver object, used to manage a resolver that Tomahawk knows about
 *
 * You *must* start() a resolver after creating an ExternalResolver in order to use it,
 * otherwise it will not do anything.
 */
class DLLEXPORT ExternalResolver : public Resolver
{
Q_OBJECT

    friend class ScriptCommandQueue;
    friend class ScriptCommand_LookupUrl;

public:
    enum ErrorState {
        NoError,
        FileNotFound,
        FailedToLoad
    };

    enum Capability
    {
        NullCapability = 0x0,
        Browsable = 0x1,        // can be represented in one or more collection tree views
        PlaylistSync = 0x2,     // can sync playlists
        AccountFactory = 0x4,   // can configure multiple accounts at the same time
        UrlLookup = 0x8         // can be queried for information on an Url
    };
    Q_DECLARE_FLAGS( Capabilities, Capability )
    Q_FLAGS( Capabilities )

    enum UrlType
    {
        UrlTypeAny =       0x00,
        UrlTypePlaylist =  0x01,
        UrlTypeTrack =     0x02,
        UrlTypeAlbum =     0x04,
        UrlTypeArtist =    0x08,
        UrlTypeXspf =      0x10
    };
    Q_DECLARE_FLAGS( UrlTypes, UrlType )
    Q_FLAGS( UrlTypes )

    ExternalResolver( const QString& filePath )
        : m_commandQueue( new ScriptCommandQueue( this ) )
    { m_filePath = filePath; }

    QString filePath() const { return m_filePath; }
    virtual void setIcon( const QPixmap& ) {}

    virtual void saveConfig() = 0;

    virtual void reload() {} // Reloads from file (especially useful to check if file now exists)
    virtual ErrorState error() const;
    virtual bool running() const = 0;
    virtual Capabilities capabilities() const = 0;

    // UrlLookup, sync call
    virtual bool canParseUrl( const QString& url, UrlType type ) = 0;

    virtual void enqueue( const QSharedPointer< ScriptCommand >& req )
    { m_commandQueue->enqueue( req ); }

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void changed(); // if config widget was added/removed, name changed, etc

    void artistsFound( const QList< Tomahawk::artist_ptr >& );
    void albumsFound( const QList< Tomahawk::album_ptr >& );
    void tracksFound( const QList< Tomahawk::query_ptr >& );
    void informationFound( const QString&, const QSharedPointer<QObject>& );

protected:
    void setFilePath( const QString& path ) { m_filePath = path; }
    ScriptCommandQueue* m_commandQueue;

    // Should only be called by ScriptCommands
    // UrlLookup
    virtual void lookupUrl( const QString& url ) = 0;

private:
    QString m_filePath;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( ExternalResolver::Capabilities )

} //ns

#endif // EXTERNALESOLVER_H
