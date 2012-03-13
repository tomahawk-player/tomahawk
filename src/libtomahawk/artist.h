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

#ifndef TOMAHAWKARTIST_H
#define TOMAHAWKARTIST_H

#include "config.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#ifndef ENABLE_HEADLESS
    #include <QtGui/QPixmap>
#endif

#include "typedefs.h"
#include "dllmacro.h"
#include "infosystem/infosystem.h"

namespace Tomahawk
{

class DLLEXPORT Artist : public QObject
{
Q_OBJECT

public:
    static artist_ptr get( const QString& name, bool autoCreate = false );
    static artist_ptr get( unsigned int id, const QString& name );

    explicit Artist( unsigned int id, const QString& name );
    virtual ~Artist();

    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }
    QString sortname() const { return m_sortname; }
#ifndef ENABLE_HEADLESS
    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
#endif
    bool infoLoaded() const { return m_infoLoaded; }

    Tomahawk::playlistinterface_ptr playlistInterface();

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks );
    void updated();

private slots:
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

private:
    Q_DISABLE_COPY( Artist )

    unsigned int m_id;
    QString m_name;
    QString m_sortname;
    QByteArray m_coverBuffer;
    mutable QPixmap* m_cover;
    bool m_infoLoaded;
    mutable QString m_uuid;

    mutable QHash< int, QPixmap > m_coverCache;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

} // ns

#endif
