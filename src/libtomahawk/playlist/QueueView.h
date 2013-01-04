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

#ifndef QUEUEVIEW_H
#define QUEUEVIEW_H

#include <QPushButton>

#include "widgets/AnimatedSplitter.h"

#include "DllMacro.h"

class QTimer;
class PlaylistView;

namespace Ui
{
    class QueueView;
}

class DLLEXPORT QueueView : public AnimatedWidget
{
Q_OBJECT

public:
    explicit QueueView( AnimatedSplitter* parent );
    ~QueueView();

    PlaylistView* queue() const;

    QSize sizeHint() const { return QSize( 0, 200 ); }

    virtual bool eventFilter( QObject* , QEvent* );

public slots:
    virtual void onShown( QWidget*, bool animated );
    virtual void onHidden( QWidget*, bool animated );

    virtual void show();
    virtual void hide();

protected:
    void changeEvent( QEvent* e );
    
private slots:
    void updateLabel();
    void onAnimationFinished();

private:
    Ui::QueueView* ui;
    QTimer* m_dragTimer;
};

#endif // QUEUEVIEW_H
