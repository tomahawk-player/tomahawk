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

#ifndef PROGRESSTREEVIEW_H
#define PROGRESSTREEVIEW_H

#include <QTreeView>
#include <QProgressBar>

#include "dllmacro.h"

class DLLEXPORT ProgressTreeView : public QTreeView
{
Q_OBJECT

public:
    ProgressTreeView( QWidget* parent );

    void connectProgressBar( QProgressBar* progressBar ) { m_progressBar = progressBar; progressBar->setVisible( false ); }

    void setProgressStarted( int length ) { if ( m_progressBar ) { m_progressBar->setRange( 0, length ); m_progressBar->setValue( 0 ); m_progressBar->setVisible( true ); } }
    void setProgressEnded() { if ( m_progressBar ) m_progressBar->setVisible( false );  }
    void setProgressCompletion( int completion ) { if ( m_progressBar ) m_progressBar->setValue( completion ); }

private:
    QProgressBar* m_progressBar;
};

#endif // PROGRESSTREEVIEW_H
