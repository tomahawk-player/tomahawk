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

#include "ReadOrWriteWidget.h"

#include <QLabel>
#include <QStackedLayout>

#include "utils/Logger.h"


ReadOrWriteWidget::ReadOrWriteWidget( QWidget* writableWidget, bool writable, QWidget* parent)
    : QWidget( parent )
    , m_writableWidget( writableWidget )
    , m_label( 0 )
    , m_layout( 0 )
    , m_writable( writable )
{
    m_label = new QLabel( QString(), this );

    m_layout = new QStackedLayout( this );
    if( writableWidget )
        m_layout->addWidget( writableWidget );

    m_layout->addWidget( m_label );

    setWritable( m_writable );

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSpacing( 0 );
}


void
ReadOrWriteWidget::setWritable( bool write )
{
    m_writable = write;
    if( m_writableWidget && write )
        m_layout->setCurrentWidget( m_writableWidget );
    else
        m_layout->setCurrentWidget( m_label );
}


void
ReadOrWriteWidget::setWritableWidget( QWidget* w )
{
    if( m_writableWidget ) {
        m_layout->removeWidget( m_writableWidget );
    }

    m_writableWidget = w;
    m_layout->insertWidget( 0, m_writableWidget );
}


bool
ReadOrWriteWidget::writable() const
{
    return m_writable;
}


QWidget*
ReadOrWriteWidget::writableWidget() const
{
    return m_writableWidget;
}


QString
ReadOrWriteWidget::label() const
{
    return m_label->text();
}


void
ReadOrWriteWidget::setLabel( const QString& label )
{
    m_label->setText( label );
}


QSize
ReadOrWriteWidget::sizeHint() const
{
    if( m_writableWidget ) {
        return m_writableWidget->sizeHint();
    } else {
        return m_label->sizeHint();
    }
}

