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

#include "DatabaseGenerator.h"

#include "DatabaseControl.h"
#include "utils/Logger.h"
#include "Source.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "database/Database.h"

using namespace Tomahawk;


GeneratorInterface*
DatabaseFactory::create()
{
    return new DatabaseGenerator();
}


dyncontrol_ptr
DatabaseFactory::createControl ( const QString& controlType )
{
    return dyncontrol_ptr( new DatabaseControl( controlType, typeSelectors() ) );
}


dyncontrol_ptr
DatabaseFactory::createControl ( const QString& sql, DatabaseCommand_GenericSelect::QueryType type, const QString& summary )
{
    dyncontrol_ptr control = dyncontrol_ptr( new DatabaseControl( sql, summary, typeSelectors() ) );
    control->setMatch( QString::number( type ) );
    return control;
}


QStringList
DatabaseFactory::typeSelectors() const
{
    return QStringList() << "SQL" << "Artist" << "Album" << "Title";
}


DatabaseGenerator::DatabaseGenerator ( QObject* parent )
    : GeneratorInterface ( parent )
{
    // defaults
    m_type = "database";
    m_mode = Static;
//     m_logo.load( RESPATH "images )
}


DatabaseGenerator::~DatabaseGenerator()
{
}


QPixmap
DatabaseGenerator::logo()
{
    return m_logo;
}


void
DatabaseGenerator::dynamicFetched()
{
}


void
DatabaseGenerator::dynamicStarted()
{
}


void
DatabaseGenerator::generate( int number )
{
    tLog() << "Generating" << number << "tracks for this database dynamic playlist with" << m_controls.size() <<  "controls:";
    if ( m_controls.isEmpty() )
    {
        qWarning() << "No controls, can't generate...!";
        emit error( "Failed to generate tracks", "No controls!" );
        return;
    }

    foreach ( const dyncontrol_ptr& ctrl, m_controls )
        qDebug() << ctrl->selectedType() << ctrl->match() << ctrl->input();

    // TODO for now, we just support the special "SQL" control, not meant to be shown to the user. Just does a raw query.
    bool hasSql = false;
    bool hasOther = false;
    foreach ( const dyncontrol_ptr& ctrl, m_controls )
    {
        if ( ctrl->selectedType() == "SQL" )
            hasSql = true;
        else
            hasOther = true;
    }
    if ( hasSql == hasOther )
    {
        qWarning() << "Cannot mix sql and non-sql controls!";
        emit error( "Failed to generate tracks", "Cannot mix sql and non-sql controls" );
        return;
    }

    // TODO for now we just support 1 sql query if we're a sql type
    if ( hasSql )
    {
        dyncontrol_ptr control = m_controls.first();

        tDebug() << "Generated sql query:" << control.dynamicCast< DatabaseControl >()->sql();
        DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( control.dynamicCast< DatabaseControl >()->sql(),
                                                                                static_cast< DatabaseCommand_GenericSelect::QueryType >( control->match().toInt() ),
                                                                                number
                                                                              );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );

        return;
    }

}


void
DatabaseGenerator::tracksGenerated ( const QList< query_ptr >& tracks )
{
    emit generated( tracks );
}


dyncontrol_ptr
DatabaseGenerator::createControl( const QString& type )
{
    m_controls << dyncontrol_ptr( new DatabaseControl( type, GeneratorFactory::typeSelectors( m_type ) ) );
    return m_controls.last();
}


dyncontrol_ptr
DatabaseGenerator::createControl ( const QString& sql, DatabaseCommand_GenericSelect::QueryType type, const QString& summary )
{
    m_controls << dyncontrol_ptr( new DatabaseControl( sql, summary, GeneratorFactory::typeSelectors( m_type ) ) );
    m_controls.last()->setMatch( QString::number( type ) );
    return m_controls.last();
}


void
DatabaseGenerator::fetchNext( int /* rating */ )
{
}


QString
DatabaseGenerator::sentenceSummary()
{
    if( m_controls.count() && m_controls.first()->type() == "SQL" )
        return m_controls.first()->summary();

    // TODO
    return QString();
}


void
DatabaseGenerator::startOnDemand()
{
}
