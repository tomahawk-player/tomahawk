/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DYNAMIC_MODEL_H
#define DYNAMIC_MODEL_H

#include "playlist/PlaylistModel.h"
#include "Query.h"

namespace Tomahawk
{

class StationModelItem;

/**
 * Extends PlaylistModel with support for handling stations
 */
class DynamicModel : public PlaylistModel
{
    Q_OBJECT
public:
    DynamicModel( QObject* parent = 0 );
    virtual ~DynamicModel();

    void startOnDemand();
    void stopOnDemand( bool stopPlaying = true );
    void changeStation();

    virtual QString description() const;

    void loadPlaylist( const dynplaylist_ptr& playlist, bool loadEntries = true );

    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );

    bool searchingForNext() const { return m_searchingForNext; }

    void setFilterUnresolvable( bool filter ) { m_filterUnresolvable = filter; }
    bool filterUnresolvable() const { return m_filterUnresolvable; }

    // a batchof static tracks wre generated
    void tracksGenerated( const QList< query_ptr > entries, int limitResolvedTo = -1 );

    using PlaylistModel::loadPlaylist;

    bool ignoreRevision( const QString& revisionguid ) const { return waitForRevision( revisionguid ); }
    void removeRevisionFromIgnore( const QString& revisionguid ) { removeFromWaitList( revisionguid ); }

signals:
    void collapseFromTo( int startRow, int num );
    void checkForOverflow();

    void trackGenerationFailure( const QString& msg );

    void tracksAdded();

private slots:
    void newTrackGenerated( const Tomahawk::query_ptr& query );

    void trackResolveFinished( bool );
    void newTrackLoading();

    void filteringTrackResolved( bool successful );

private:
    void filterUnresolved( const QList< query_ptr >& entries );
    void addToPlaylist( const QList< query_ptr >& entries, bool clearFirst );

    dynplaylist_ptr m_playlist;

    // for filtering unresolvable
    int m_limitResolvedTo;
    QList< query_ptr > m_toResolveList;
    QList< query_ptr > m_resolvedList;

    // for managing upcoming queue
    QList< Query* > m_waitingFor;
    QList< QPair< QString, QString > > m_deduper;

    bool m_onDemandRunning;
    bool m_changeOnNext;
    bool m_searchingForNext;
    bool m_firstTrackGenerated;
    bool m_filterUnresolvable;
    bool m_startingAfterFailed;
    int m_currentAttempts;
    int m_lastResolvedRow;
};

};

#endif
