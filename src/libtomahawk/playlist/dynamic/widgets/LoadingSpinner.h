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

#ifndef LOADING_SPINNER_H
#define LOADING_SPINNER_H

#include <QWidget>

class QMovie;
class QTimeLine;
/**
 * A small widget that displays an animated loading spinner
 */
class LoadingSpinner : public QWidget {
    Q_OBJECT
public:
    LoadingSpinner( QWidget* parent );
    virtual ~LoadingSpinner();
    
    virtual QSize sizeHint() const;
    virtual void paintEvent( QPaintEvent* );
    virtual void resizeEvent( QResizeEvent* );
    
public slots:        
    void fadeIn();
    void fadeOut();
    
private slots:
    void hideFinished();
    
private:
    void reposition();
    
    QTimeLine* m_showHide;
    QMovie* m_anim;
};

#endif

class QPaintEvent;