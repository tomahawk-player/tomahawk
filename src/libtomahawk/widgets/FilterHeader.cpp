/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "FilterHeader.h"

#include "thirdparty/Qocoa/qsearchfield.h"

#include <QBoxLayout>

FilterHeader::FilterHeader( QWidget* parent )
    : BasicHeader( parent )
{
    m_filterField = new QSearchField( this );
    m_filterField->setPlaceholderText( tr( "Filter..." ) );
    m_filterField->setFixedWidth( 220 );
    m_mainLayout->addWidget( m_filterField );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
    connect( m_filterField, SIGNAL( textChanged( QString ) ), SLOT( onFilterEdited() ) );
}


FilterHeader::~FilterHeader()
{
}


void
FilterHeader::setFilter( const QString& filter )
{
    m_filterField->setText( filter );
}


void
FilterHeader::onFilterEdited()
{
    m_filter = m_filterField->text();

    m_filterTimer.stop();
    m_filterTimer.setInterval( 280 );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
FilterHeader::applyFilter()
{
    emit filterTextChanged( m_filterField->text() );
}
