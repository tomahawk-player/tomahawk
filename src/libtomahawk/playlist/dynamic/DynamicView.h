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

#ifndef DYNAMIC_VIEW_H
#define DYNAMIC_VIEW_H

#include "playlist/PlaylistView.h"
#include <QTimer>
#include <QPropertyAnimation>
#include <QTimeLine>
#include <QMutex>

class PlaylistModel;
class TrackModel;
namespace Tomahawk
{

class DynamicModel;


class DynamicView : public PlaylistView
{
    Q_OBJECT
public:
    explicit DynamicView( QWidget* parent = 0 );
    virtual ~DynamicView();

    virtual void setDynamicModel( DynamicModel* model );

    void setOnDemand( bool onDemand );
    void setReadOnly( bool readOnly );

    void setDynamicWorking( bool working );

    virtual void paintEvent( QPaintEvent* event );

public slots:
    void showMessageTimeout( const QString& title, const QString& body );
    void showMessage( const QString& message );

    // collapse and animate the transition
    // there MUST be a row *after* startRow + num. that is, you can't collapse
    // entries unless there is at least one entry after the last collapsed row
    // optionally you can specify how  many rows are past the block of collapsed rows
    void collapseEntries( int startRow, int num, int numToKeep = 1 );

private slots:
    void onTrackCountChanged( unsigned int );
    void checkForOverflow();
    void animFinished();

private:
    QPixmap backgroundBetween( QRect rect, int rowStart );

    DynamicModel* m_model;
    QString m_title;
    QString m_body;

    bool m_onDemand;
    bool m_readOnly;
    bool m_checkOnCollapse;
    bool m_working;

    // for collapsing animation
    QPoint m_fadingPointAnchor;
    QPoint m_bottomAnchor;
    QPoint m_bottomOfAnim;
    QPixmap m_fadingIndexes;
    QPixmap m_slidingIndex;
    QPixmap m_bg;
    bool m_fadebg;
    bool m_fadeOnly;
    QTimeLine m_fadeOutAnim;
    QTimeLine m_slideAnim;
};

};


#endif
