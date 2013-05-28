/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef INBOXMODEL_H
#define INBOXMODEL_H

#include "PlaylistModel.h"
// #include "Typedefs.h"
#include "jobview/InboxJobItem.h"

#include "DllMacro.h"

namespace Tomahawk
{
    class SocialAction;
};

class DLLEXPORT InboxModel : public PlaylistModel
{
    Q_OBJECT
public:
    explicit InboxModel( QObject* parent = 0 );
    virtual ~InboxModel();

    virtual int unlistenedCount() const;

public slots:
    /**
     * Reimplemented from PlaylistModel, all track insertions/appends go through this method.
     * On top of PlaylistModel functionality, adds deduplication/grouping of equivalent tracks
     * sent from different sources.
     */
    virtual void insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row = 0, const QList< Tomahawk::PlaybackLog >& logs = QList< Tomahawk::PlaybackLog >() );

    virtual void removeIndex( const QModelIndex &index, bool moreToCome );

    virtual void clear();

    virtual void showNotification( InboxJobItem::Side side,
                                   const Tomahawk::source_ptr& src,
                                   const Tomahawk::trackdata_ptr& track ); //for lack of a better place to put this
    virtual void showNotification( InboxJobItem::Side side,
                                   const QString& dbid,
                                   const Tomahawk::trackdata_ptr& track );


private slots:
    void loadTracks();

    void tracksLoaded( QList< Tomahawk::query_ptr > );

private:
    static QList< Tomahawk::SocialAction > mergeSocialActions( QList< Tomahawk::SocialAction > first,
                                                               QList< Tomahawk::SocialAction > second );
};

#endif // INBOXMODEL_H
