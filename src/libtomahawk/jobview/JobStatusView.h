/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef JOBSTATUSVIEW_H
#define JOBSTATUSVIEW_H

#include "typedefs.h"
#include "widgets/animatedsplitter.h"
#include "dllmacro.h"

class QAbstractItemModel;
class QListView;
class JobStatusModel;
class StreamConnection;

class DLLEXPORT JobStatusView : public AnimatedWidget
{
Q_OBJECT

public:
    static JobStatusView* instance() {
        return s_instance;
    }

    explicit JobStatusView( AnimatedSplitter* parent );
    virtual ~JobStatusView()
    {
    }

    QSize sizeHint() const;

    void setModel( JobStatusModel* model );

    JobStatusModel* model() { return m_model; }

private slots:
    void checkCount();

private:
    QListView* m_view;
    JobStatusModel* m_model;
    AnimatedSplitter* m_parent;

    static JobStatusView* s_instance;
};

#endif // JOBSTATUSVIEW_H
