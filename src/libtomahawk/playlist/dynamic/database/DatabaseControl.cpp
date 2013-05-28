/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DatabaseControl.h"

#include <QWidget>

using namespace Tomahawk;

DatabaseControl::DatabaseControl( const QString& selectedType, const QStringList& typeSelectors, QObject* parent )
: DynamicControl ( selectedType.isEmpty() ? "Artist" : selectedType, typeSelectors, parent )
{
    setType( "database" );

    m_editingTimer.setInterval( 500 ); //timeout to edits
    m_editingTimer.setSingleShot( true );
    connect( &m_editingTimer, SIGNAL( timeout() ), this, SLOT( editTimerFired() ) );

    m_delayedEditTimer.setInterval( 250 ); // additional timer for "just typing" without enter or focus change
    m_delayedEditTimer.setSingleShot( true );
    connect( &m_delayedEditTimer, SIGNAL( timeout() ), &m_editingTimer, SLOT( start() ) );

}

DatabaseControl::DatabaseControl( const QString& sql, const QString& summary, const QStringList& typeSelectors, QObject* parent )
    : DynamicControl ( "SQL", typeSelectors )
    , m_sql( sql )
    , m_sqlSummary( summary )
{
    Q_UNUSED( parent );
    setType( "database" );
}


QString DatabaseControl::input() const
{
    // TODO
    return QString();
}

QWidget* DatabaseControl::inputField()
{
    return 0;
}

QString DatabaseControl::match() const
{
    return m_matchData;
}

QWidget* DatabaseControl::matchSelector()
{
    return 0;
}

QString DatabaseControl::matchString() const
{
    return m_matchString;
}

void DatabaseControl::setInput ( const QString& input )
{
    Q_UNUSED( input );
    // TODO
    updateWidgets();
}

void DatabaseControl::setMatch ( const QString& match )
{
    m_matchData = match;

    updateWidgets();
}

void DatabaseControl::setSelectedType ( const QString& type )
{
    if ( type != selectedType() ) {
        if ( !m_input.isNull() )
            delete m_input.data();
        if ( !m_match.isNull() )
            delete m_match.data();

        Tomahawk::DynamicControl::setSelectedType ( type );
        updateWidgets();
        updateData();
        //        qDebug() << "Setting new type, set data to:" << m_data.first << m_data.second;
    }
}

void DatabaseControl::editingFinished()
{

}

void DatabaseControl::editTimerFired()
{

}

void DatabaseControl::updateData()
{

}

void DatabaseControl::updateWidgets()
{

}

void DatabaseControl::updateWidgetsFromData()
{

}

void DatabaseControl::calculateSummary()
{
    if( !m_sqlSummary.isEmpty() )
        return;

}

QString
DatabaseControl::sql() const
{
    return m_sql;
}


QString
DatabaseControl::summary() const
{
    if( !m_sqlSummary.isEmpty() )
        return m_sqlSummary;

    return m_summary;
}
