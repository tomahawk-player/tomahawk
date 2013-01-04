/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#include "AccountListWidget.h"

#include "accounts/AccountModel.h"
#include "accounts/AccountManager.h"
#include "AccountWidget.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPushButton>


AccountListWidget::AccountListWidget( AccountModelFactoryProxy* model, QWidget* parent )
    : QWidget( parent )
    , m_model( model )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    TomahawkUtils::unmarginLayout( mainLayout );
    m_layout = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( m_layout );
    mainLayout->addLayout( m_layout );
    mainLayout->setSpacing( 8 );

    connect( m_model, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
             this, SLOT( updateEntries( QModelIndex, QModelIndex ) ) );
    connect( m_model, SIGNAL( rowsInserted ( QModelIndex, int, int ) ),
             this, SLOT( insertEntries( QModelIndex, int, int ) ) );
    connect( m_model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ),
             this, SLOT( removeEntries( QModelIndex, int, int ) ) );
    connect( m_model, SIGNAL( modelReset() ),
             this, SLOT( loadAllEntries() ) );

    connect( m_model, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
             this, SLOT( updateToggleOnlineStateButton() ) );

    QWidget* separatorLine = new QWidget( this );
    separatorLine->setFixedHeight( 1 );
    separatorLine->setContentsMargins( 0, 0, 0, 0 );
    separatorLine->setStyleSheet( "QWidget { border-top: 1px solid " +
                                  TomahawkUtils::Colors::BORDER_LINE.name() + "; }" ); //from ProxyStyle
    mainLayout->insertWidget( 0, separatorLine );
    mainLayout->addSpacing( 6 );

    QLabel *connectionsLabel = new QLabel( tr( "Connections" ).toUpper(), this );
    QFont clFont = connectionsLabel->font();
    clFont.setBold( true );
    connectionsLabel->setStyleSheet( "color: " + TomahawkUtils::Colors::GROUP_HEADER.name() );
    clFont.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    connectionsLabel->setFont( clFont );
    connectionsLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    m_toggleOnlineButton = new QPushButton( tr( "Connect &All" ), this );
    m_toggleOnlineButtonState = false;
    connect( m_toggleOnlineButton, SIGNAL( clicked() ),
             this, SLOT( toggleOnlineStateForAll() ) );

    QHBoxLayout *headerLayout = new QHBoxLayout( this );
    headerLayout->addWidget( connectionsLabel );
    headerLayout->addSpacing( 30 );
    headerLayout->addWidget( m_toggleOnlineButton );
    mainLayout->insertLayout( 0, headerLayout );

    updateToggleOnlineStateButton();
}

void
AccountListWidget::updateEntries( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    for( int row = topLeft.row(); row <= bottomRight.row(); ++row )
    {
        QPersistentModelIndex idx( m_model->index( row, 0 ) );

        int newCount = idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
                            .value< QList< Tomahawk::Accounts::Account* > >().count();

        if( m_entries.value( idx ).count() == newCount )
        {
            updateEntry( idx );
        }
        else
        {
            removeEntries( idx.parent(), idx.row(), idx.row() );
            insertEntries( idx.parent(), idx.row(), idx.row() );
        }
    }
}

void
AccountListWidget::updateEntry( const QPersistentModelIndex& idx )
{
    for ( int i = 0; i < m_entries.value( idx ).count(); ++i )
    {
        m_entries[ idx ][ i ]->update( idx, i ); //update the i-th account of the idx-th factory
    }
}

void
AccountListWidget::loadAllEntries()
{
    foreach ( QList< AccountWidget* > entry, m_entries )
    {
        foreach ( AccountWidget* w, entry )
        {
            m_layout->removeWidget( w );
            w->deleteLater();
        }
        entry.clear();
    }
    m_entries.clear();

    int rc =  m_model->rowCount();
    insertEntries( QModelIndex(), 0, rc - 1 );
}

void
AccountListWidget::insertEntries(  const QModelIndex& parent, int start, int end )
{
    for ( int i = start; i <= end; ++i )
    {
        QPersistentModelIndex idx( m_model->index( i, 0, parent ) );
        int count = idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
                .value< QList< Tomahawk::Accounts::Account* > >().count();
        QList< AccountWidget* > entryAccounts;
        for ( int j = 0; j < count; ++j )
        {
            AccountWidget *entry = new AccountWidget( this );
            entryAccounts.append( entry );
        }
        m_entries.insert( idx, entryAccounts );
        for ( int j = 0; j < entryAccounts.length(); ++j )
            m_layout->insertWidget( i+j, entryAccounts.at( j ) );

        updateEntry( idx );

        for ( int j = 0; j < entryAccounts.length(); ++j )
        {
            entryAccounts[ j ]->setupConnections( idx, j );
        }
    }
}

void
AccountListWidget::removeEntries( const QModelIndex& parent, int start, int end )
{
    for ( int i = start; i <= end; ++i )
    {
        QPersistentModelIndex idx( m_model->index( i, 0, parent ) );
        if ( !idx.isValid() ) //means we just removed the last account for a factory
        {
            for ( QHash< QPersistentModelIndex, QList< AccountWidget* > >::iterator it = m_entries.begin();
                  it != m_entries.end(); ++it )
            {
                if ( !it.key().isValid() )
                {
                    idx = it.key();
                }
            }
        }

        QList< AccountWidget* > &entryAccounts = m_entries[ idx ];
        for ( int j = 0; j < entryAccounts.count(); ++j )
        {
            AccountWidget *a = entryAccounts.at( j );
            m_layout->removeWidget( a );
            a->deleteLater();
        }
        m_entries.remove( idx );
    }
    adjustSize();
    qobject_cast< QWidget* >( QWidget::parent() )->adjustSize();
}

void
AccountListWidget::toggleOnlineStateForAll()
{
    bool newState = !m_toggleOnlineButtonState;
    foreach ( QList< AccountWidget* > awgts, m_entries )
    {
        foreach ( AccountWidget* awgt, awgts )
        {
            awgt->setConnectionState( newState );
        }
    }
}

void
AccountListWidget::updateToggleOnlineStateButton()
{
    bool newState = false;
    foreach ( QList< AccountWidget* > awgts, m_entries )
    {
        foreach ( AccountWidget* awgt, awgts )
        {
            if ( awgt->connectionState() )
            {
                newState = true;
                goto end; //break 2 levels
            }
        }
    }
    end:;

    if ( newState != m_toggleOnlineButtonState )
    {
        m_toggleOnlineButtonState = newState;
        if ( newState )
            Tomahawk::Accounts::AccountManager::instance()->connectAll();
        else
            Tomahawk::Accounts::AccountManager::instance()->disconnectAll();
    }

    m_toggleOnlineButton->setText( m_toggleOnlineButtonState ? tr( "Disconnect &All" )
                                                             : tr( "Connect &All" ) );
    m_toggleOnlineButton->setIcon( m_toggleOnlineButtonState ?
                                        ImageRegistry::instance()->icon( RESPATH "images/account-online.svg" ) :
                                        ImageRegistry::instance()->icon( RESPATH "images/account-offline.svg" ) );
}
