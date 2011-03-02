#include "databasecommand_dirmtimes.h"

#include <QSqlQuery>

#include "databaseimpl.h"


void
DatabaseCommand_DirMtimes::exec( DatabaseImpl* dbi )
{
    if( m_update )
        execUpdate( dbi );
    else
        execSelect( dbi );
}


void
DatabaseCommand_DirMtimes::execSelect( DatabaseImpl* dbi )
{
    QMap<QString,unsigned int> mtimes;
    TomahawkSqlQuery query = dbi->newquery();
    if( m_prefix.isEmpty() )
        query.exec( "SELECT name, mtime FROM dirs_scanned" );
    else
    {
        query.prepare( QString( "SELECT name, mtime "
                                "FROM dirs_scanned "
                                "WHERE name LIKE '%1%'" ).arg( m_prefix.replace( '\'',"''" ) ) );
        query.exec();
    }
    while( query.next() )
    {
        mtimes.insert( query.value( 0 ).toString(), query.value( 1 ).toUInt() );
    }

    emit done( mtimes );
}


void
DatabaseCommand_DirMtimes::execUpdate( DatabaseImpl* dbi )
{
    qDebug() << "Saving mtimes...";
    TomahawkSqlQuery query = dbi->newquery();
    query.exec( "DELETE FROM dirs_scanned" );
    query.prepare( "INSERT INTO dirs_scanned(name, mtime) VALUES(?, ?)" );

    foreach( const QString& k, m_tosave.keys() )
    {
        query.bindValue( 0, k );
        query.bindValue( 1, m_tosave.value( k ) );
        query.exec();
    }

    qDebug() << "Saved mtimes for" << m_tosave.size() << "dirs.";
}
