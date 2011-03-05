/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DYNAMIC_MODEL_H
#define DYNAMIC_MODEL_H

#include "playlistmodel.h"
#include "query.h"

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
    
    void loadPlaylist( const dynplaylist_ptr& playlist );
    
    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );
    
    bool searchingForNext() const { return m_searchingForNext; }
    
    void setFilterUnresolvable( bool filter ) { m_filterUnresolvable = filter; }
    bool filterUnresolvable() const { return m_filterUnresolvable; }
    
    // a batchof static tracks wre generated
    void tracksGenerated( const QList< query_ptr > entries, int limitResolvedTo = -1 );
signals:
    void collapseFromTo( int startRow, int num );
    void checkForOverflow();

    void trackGenerationFailure( const QString& msg );
    
    void tracksAdded();
private slots:    
    void newTrackGenerated( const Tomahawk::query_ptr& query );
    
    void trackResolveFinished( bool );
    void trackResolved();
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
    
    bool m_startOnResolved;
    bool m_onDemandRunning;
    bool m_changeOnNext;
    bool m_searchingForNext;
    bool m_firstTrackGenerated;
    bool m_filterUnresolvable;
    int m_currentAttempts;
    int m_lastResolvedRow;
};
    
};

#endif
