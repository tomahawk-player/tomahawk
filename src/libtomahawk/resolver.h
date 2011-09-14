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

#ifndef RESOLVER_H
#define RESOLVER_H

#include <QObject>

#include "query.h"

#include "dllmacro.h"

// implement this if you can resolve queries to content

/*
    Weight: 1-100, 100 being the best
    Timeout: some millisecond value, after which we try the next highest
             weighted resolver

*/

class QWidget;

namespace Tomahawk
{

class DLLEXPORT Resolver : public QObject
{
Q_OBJECT

public:
    Resolver() {}

    virtual QString name() const = 0;
    virtual unsigned int weight() const = 0;
    virtual unsigned int timeout() const = 0;

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query ) = 0;
};

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

    ExternalResolver( const QString& filePath ) { m_filePath = filePath; }

    virtual QString filePath() const { return m_filePath; }

    virtual QWidget* configUI() const = 0;
    virtual void saveConfig() = 0;

    virtual void reload() {} // Reloads from file (especially useful to check if file now exists)
    virtual ErrorState error() const;
    virtual bool running() const = 0;

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void changed(); // if config widget was added/removed

protected:
    QWidget* widgetFromData( QByteArray& data, QWidget* parent = 0 );
    QVariant configMsgFromWidget( QWidget* w );
    QByteArray fixDataImagePaths( const QByteArray& data, bool compressed, const QVariantMap& images );

    void setFilePath( const QString& path ) { m_filePath = path; }

private:
    void addChildProperties( QObject* parent, QVariantMap& m );

    QString m_filePath;
};

}; //ns

#endif // RESOLVER_H
