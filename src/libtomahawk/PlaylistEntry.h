/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef PLAYLISTENTRY_H
#define PLAYLISTENTRY_H

#include <QObject>
#include <QList>
#include <QVariant>
#include <QSharedPointer>

#include "Typedefs.h"
#include "Query.h"

#include "DllMacro.h"

namespace Tomahawk
{

class PlaylistEntryPrivate;

class DLLEXPORT PlaylistEntry : public QObject
{
Q_OBJECT
Q_PROPERTY( QString guid              READ guid         WRITE setGuid )
Q_PROPERTY( QString annotation        READ annotation   WRITE setAnnotation )
Q_PROPERTY( unsigned int duration     READ duration     WRITE setDuration )
Q_PROPERTY( unsigned int lastmodified READ lastmodified WRITE setLastmodified )
Q_PROPERTY( QVariant query            READ queryVariant WRITE setQueryVariant )

public:
    PlaylistEntry();
    virtual ~PlaylistEntry();

    bool isValid() const;

    void setQuery( const Tomahawk::query_ptr& q );
    const Tomahawk::query_ptr& query() const;

    void setQueryVariant( const QVariant& v );
    QVariant queryVariant() const;

    QString guid() const;
    void setGuid( const QString& s );

    QString annotation() const;
    void setAnnotation( const QString& s );

    QString resultHint() const;
    void setResultHint( const QString& s );

    unsigned int duration() const;
    void setDuration( unsigned int i );

    unsigned int lastmodified() const;
    void setLastmodified( unsigned int i );

    source_ptr lastSource() const;
    void setLastSource( source_ptr s );

signals:
    void resultChanged();

private slots:
    void onQueryResolved( bool hasResults );

private:
    Q_DECLARE_PRIVATE( PlaylistEntry )
    QScopedPointer<PlaylistEntryPrivate> d_ptr;

    QString hintFromQuery() const;
};

}

Q_DECLARE_METATYPE( QList< QSharedPointer< Tomahawk::PlaylistEntry > > )

#endif // PLAYLISTENTRY_H
