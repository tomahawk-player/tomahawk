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

#ifndef DYNAMIC_VIEW_H
#define DYNAMIC_VIEW_H

#include "playlist/playlistview.h"
#include <QTimer>
#include <QPropertyAnimation>

class PlaylistModel;
class TrackModel;
namespace Tomahawk
{
    
class DynamicView : public PlaylistView
{
    Q_OBJECT
public:
    explicit DynamicView( QWidget* parent = 0 );
    virtual ~DynamicView();
    
    virtual void setModel( PlaylistModel* model );
    
    void setOnDemand( bool onDemand );
public slots:
    void showMessageTimeout( const QString& title, const QString& body );
    
private slots:
    void onTrackCountChanged( unsigned int );
    
private:
    QTimer m_showTimer;
    QPropertyAnimation* m_fadeOut;
    
    QString m_title;
    QString m_body;
    
    bool m_onDemand;
};
    
};


#endif
