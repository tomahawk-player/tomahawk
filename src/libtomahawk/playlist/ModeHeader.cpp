/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012-2014, Teo Mrnjavac <teo@kde.org>
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

#include "ModeHeader.h"

#include <QBoxLayout>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QCheckBox>
#include <QPaintEvent>
#include <QPainter>
#include <QRadioButton>

#include "playlist/FlexibleView.h"
#include "ViewManager.h"
#include "utils/TomahawkStyle.h"
#include "utils/Closure.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "widgets/QueryLabel.h"
#include "Source.h"

using namespace Tomahawk;


ModeHeader::ModeHeader( QWidget* parent )
    : QWidget( parent )
    , m_parent( parent )
{
    QFile f( RESPATH "stylesheets/topbar-radiobuttons.css" );
    f.open( QFile::ReadOnly );
    QString css = QString::fromLatin1( f.readAll() );
    f.close();

    QHBoxLayout* outerModeLayout = new QHBoxLayout;
    outerModeLayout->addStretch();

    QWidget* modeWidget = new QWidget( this );
    QHBoxLayout* modeLayout = new QHBoxLayout;
    modeWidget->setLayout( modeLayout );
    modeWidget->setStyleSheet( css );

    m_radioNormal = new QRadioButton( modeWidget );
    m_radioDetailed = new QRadioButton( modeWidget );
    m_radioCloud = new QRadioButton( modeWidget );
    //for the CSS:
    m_radioNormal->setObjectName( "radioNormal" );
    m_radioCloud->setObjectName( "radioCloud" );

    m_radioNormal->setFocusPolicy( Qt::NoFocus );
    m_radioDetailed->setFocusPolicy( Qt::NoFocus );
    m_radioCloud->setFocusPolicy( Qt::NoFocus );

    modeLayout->addWidget( m_radioNormal );
    modeLayout->addWidget( m_radioDetailed );
    modeLayout->addWidget( m_radioCloud );

    modeWidget->setFixedSize( 87, 30 );

    m_radioNormal->setChecked( true );

    outerModeLayout->addWidget( modeWidget );
    outerModeLayout->addStretch();

    setLayout( outerModeLayout );

    TomahawkUtils::unmarginLayout( outerModeLayout );
    TomahawkUtils::unmarginLayout( modeLayout );
    outerModeLayout->setContentsMargins( 2, 2, 2, 2 );

    QPalette pal = palette();
    pal.setColor( QPalette::Foreground, Qt::white );
    pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND.lighter() );

    setAutoFillBackground( true );
    setPalette( pal );

    connect( m_radioNormal,   SIGNAL( clicked() ), SIGNAL( flatClicked() ) );
    connect( m_radioDetailed, SIGNAL( clicked() ), SIGNAL( detailedClicked() ) );
    connect( m_radioCloud,    SIGNAL( clicked() ), SIGNAL( gridClicked() ) );
}


ModeHeader::~ModeHeader()
{
}


void
ModeHeader::switchTo( int buttonIndex )
{
    switch ( buttonIndex )
    {
    case 0:
        m_radioNormal->click();
        break;
    case 1:
        m_radioDetailed->click();
        break;
    case 2:
        m_radioCloud->click();
        break;
    }
}


void
ModeHeader::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}

