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

#include "ScriptCollectionHeader.h"

#include "utils/ImageRegistry.h"
#include "widgets/ElidedLabel.h"

#include <QBoxLayout>
#include <QPushButton>

ScriptCollectionHeader::ScriptCollectionHeader( QWidget* parent )
    : FilterHeader( parent )
{
    m_refreshButton = new QPushButton( this );
    m_refreshButton->setFlat( true );
    m_refreshButton->setStyleSheet( "QPushButton { border: 0px; background: transparent; }" );
    QHBoxLayout* descLayout = new QHBoxLayout;
    m_verticalLayout->insertLayout( m_verticalLayout->indexOf( m_descriptionLabel ),
                                    descLayout );
    descLayout->addWidget( m_descriptionLabel );
    TomahawkUtils::unmarginLayout( descLayout );
    descLayout->addSpacing( m_descriptionLabel->fontMetrics().height() / 2 );
    descLayout->addWidget( m_refreshButton );
    descLayout->addStretch();

    m_refreshButton->setIcon( ImageRegistry::instance()->pixmap( RESPATH "images/refresh.svg", QSize( m_descriptionLabel->fontMetrics().height(),
                                                                                                    m_descriptionLabel->fontMetrics().height() ), TomahawkUtils::DropShadow ) );
    m_refreshButton->setFixedSize( m_descriptionLabel->fontMetrics().height() + m_descriptionLabel->margin() * 2,
                                   m_descriptionLabel->fontMetrics().height() + m_descriptionLabel->margin() * 2 );

    connect( m_refreshButton, SIGNAL( clicked() ),
             this, SIGNAL( refreshClicked() ) );
    m_refreshButton->hide();
    m_refreshButton->setToolTip( tr( "Reload Collection" ) );
}


void
ScriptCollectionHeader::setRefreshVisible( bool visible )
{
    m_refreshButton->setVisible( visible );
}
