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
    void stopOnDemand();
    void changeStation();
    
    void loadPlaylist( const dynplaylist_ptr& playlist );
    
    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );
    
    bool searchingForNext() const { return m_searchingForNext; }
signals:
    void collapseFromTo( int startRow, int num );
    void checkForOverflow();

    void trackGenerationFailure( const QString& msg );
    
private slots:    
    void newTrackGenerated( const Tomahawk::query_ptr& query );
    
    void trackResolveFinished( bool );
    void trackResolved();
    void newTrackLoading();
    
private:
    dynplaylist_ptr m_playlist;
    bool m_startOnResolved;
    bool m_onDemandRunning;
    bool m_changeOnNext;
    bool m_searchingForNext;
    int m_currentAttempts;
    int m_lastResolvedRow;
};
    
};

#endif
