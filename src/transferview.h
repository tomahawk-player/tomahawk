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

#ifndef TRANSFERVIEW_H
#define TRANSFERVIEW_H

#include <QTreeWidget>

#include "typedefs.h"
#include "widgets/animatedsplitter.h"

class StreamConnection;

class TransferView : public AnimatedWidget
{
Q_OBJECT

public:
    explicit TransferView( AnimatedSplitter* parent );
    virtual ~TransferView()
    {
    }

    QSize sizeHint() const;

signals:

private slots:
    void streamRegistered( StreamConnection* sc );
    void streamFinished( StreamConnection* sc );

    void onTransferUpdate();

private:
    QHash< StreamConnection*, QPersistentModelIndex > m_index;
    QTreeWidget* m_tree;
    AnimatedSplitter* m_parent;
};

#endif // TRANSFERVIEW_H
