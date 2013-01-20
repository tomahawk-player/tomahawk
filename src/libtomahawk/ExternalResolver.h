/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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


#include "Query.h"
#include "DllMacro.h"
#include "Resolver.h"

#include <boost/function.hpp>

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
        AccountFactory = 0x4    // can configure multiple accounts at the same time
    };
    Q_DECLARE_FLAGS( Capabilities, Capability )
    Q_FLAGS( Capabilities )

    ExternalResolver( const QString& filePath ) { m_filePath = filePath; }

    virtual QString filePath() const { return m_filePath; }

    virtual void saveConfig() = 0;

    virtual void reload() {} // Reloads from file (especially useful to check if file now exists)
    virtual ErrorState error() const;
    virtual bool running() const = 0;
    virtual Capabilities capabilities() const = 0;

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void changed(); // if config widget was added/removed, name changed, etc
    void collectionAdded( const Tomahawk::collection_ptr& collection );
    void collectionRemoved( const Tomahawk::collection_ptr& collection );

protected:
    void setFilePath( const QString& path ) { m_filePath = path; }
    QList< Tomahawk::collection_ptr > m_collections;

private:
    QString m_filePath;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( ExternalResolver::Capabilities )

}; //ns

#endif // EXTERNALESOLVER_H
