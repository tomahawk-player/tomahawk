/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#include "DelegateConfigWrapper.h"
#include "AccountConfigWidget.h"

#include <QMessageBox>


DelegateConfigWrapper::DelegateConfigWrapper( AccountConfigWidget* conf, QWidget* aboutWidget, const QString& title, QWidget* parent, Qt::WindowFlags flags )
    : QDialog( parent, flags )
    , m_widget( conf )
    , m_aboutW( aboutWidget )
    , m_deleted( false )
{
    m_widget->setWindowFlags( Qt::Sheet );
#ifdef Q_WS_MAC
    m_widget->setVisible( true );
#endif
    setWindowTitle( title );
    QVBoxLayout* v = new QVBoxLayout( this );
    v->setContentsMargins( 0, 0, 0, 0 );
    v->addWidget( m_widget );

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if ( m_aboutW )
    {
        m_aboutW->hide();
        buttons |= QDialogButtonBox::Help;
    }

    m_buttons = new QDialogButtonBox( buttons, Qt::Horizontal, this );
    m_okButton = m_buttons->button( QDialogButtonBox::Ok );
    connect( m_buttons, SIGNAL( clicked( QAbstractButton*)  ), this, SLOT( closed( QAbstractButton* ) ) );
    connect( this, SIGNAL( rejected() ), this, SLOT( rejected() ) );

    if ( m_aboutW )
    {
        connect( m_buttons->button( QDialogButtonBox::Help ), SIGNAL( clicked( bool ) ), this, SLOT( aboutClicked( bool ) ) );
        m_buttons->button( QDialogButtonBox::Help )->setText( tr( "About" ) );
    }

    QHBoxLayout* h = new QHBoxLayout( this );
    h->addWidget( m_buttons );
    if( m_widget && m_widget->layout() )
        h->setContentsMargins( m_widget->layout()->contentsMargins() );
    else if( m_widget )
        h->setContentsMargins( m_widget->contentsMargins() );

    v->addLayout( h );

    setLayout( v );

#ifdef Q_WS_MAC
    setSizeGripEnabled( false );
    setMinimumSize( sizeHint() );
    setMaximumSize( sizeHint() ); // to remove the resize grip on osx this is the only way

    if( conf->metaObject()->indexOfSignal( "sizeHintChanged()" ) > -1 )
        connect( conf, SIGNAL( sizeHintChanged() ), this, SLOT( updateSizeHint() ) );
#else
    m_widget->setVisible( true );
#endif

}


void
DelegateConfigWrapper::setShowDelete( bool del )
{
    if ( del )
        m_deleteButton = m_buttons->addButton( tr( "Delete Account" ), QDialogButtonBox::DestructiveRole );
}


void
DelegateConfigWrapper::toggleOkButton( bool dataError )
{
    // if dataError is True we want to set the button enabled to false
    m_okButton->setEnabled( !dataError );
}


void
DelegateConfigWrapper::closed( QAbstractButton* b )
{
    QDialogButtonBox* buttons = qobject_cast< QDialogButtonBox* >( sender() );

    if ( buttons->standardButton( b ) == QDialogButtonBox::Help )
        return;

    int doneCode = 0;

    if ( buttons->standardButton( b ) == QDialogButtonBox::Ok )
    {
        m_widget->resetErrors();
        m_widget->checkForErrors();
        if( !m_widget->settingsValid() )
        {
            foreach( const QString& error, m_widget->errors() )
            {
                QMessageBox::warning( this, tr( "Config Error" ) , error );
            }

            return;
        }

        doneCode = QDialog::Accepted;
    }
    else if ( b == m_deleteButton )
    {
        m_deleted = true;
        emit closedWithDelete();
        reject();
        return;
    }
    else
    {
        doneCode = QDialog::Rejected;
    }

    // let the config widget live to see another day
    layout()->removeWidget( m_widget );
    m_widget->setParent( 0 );
    m_widget->setVisible( false );

    done( doneCode );
}


void
DelegateConfigWrapper::rejected()
{
    layout()->removeWidget( m_widget );
    m_widget->setParent( 0 );
    m_widget->setVisible( false );
}


void
DelegateConfigWrapper::updateSizeHint()
{
    setSizeGripEnabled( false );
    setMinimumSize( sizeHint() );
    setMaximumSize( sizeHint() );
}


void
DelegateConfigWrapper::aboutClicked( bool )
{
    Q_ASSERT( m_aboutW );
    m_aboutW->show();

    QDialog d( this );
    d.setWindowTitle( tr( "About this Account" ) );
    QVBoxLayout* v = new QVBoxLayout( &d );
    v->addWidget( m_aboutW );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok, Qt::Horizontal, &d );
    v->addWidget( bbox );

    d.setLayout( v );
    connect( bbox, SIGNAL( clicked( QAbstractButton* ) ), &d, SLOT( accept() ) );
    d.exec();
    v->removeWidget( m_aboutW );

    m_aboutW->setParent( 0 );
    m_aboutW->hide();

}

